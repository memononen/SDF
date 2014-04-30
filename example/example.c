#include <stdio.h>
#include <string.h>
#include <float.h>
#include <GLFW/glfw3.h>
#define SDF_IMPLEMENTATION
#include "sdf.h"
#include "stb_image.c"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <sys/time.h>
#include <stdint.h>

int64_t getPerfTime()
{
	struct timeval now;
	gettimeofday(&now, 0);
	return (int64_t)now.tv_sec*1000000L + (int64_t)now.tv_usec;
}

int deltaTimeUsec(int64_t start, int64_t end)
{
	return (int)(end - start);
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

struct Image {
	unsigned char* data;
	int width, height, bpp;
};

struct Image* imgLoad(const char* path, int bpp)
{
	struct Image* img = malloc(sizeof(struct Image));
	if (img == NULL) goto error;
	img->data = stbi_load(path, &img->width, &img->height, &img->bpp, bpp);
	if (img->data == NULL) goto error;
	return img;

error:
	if (img != NULL) {
		free(img->data);
		free(img);
	}
	return NULL;
}

struct Image* imgCreate(int w, int h, int bpp)
{
	struct Image* img = malloc(sizeof(struct Image));
	if (img == NULL) goto error;
	img->width = w;
	img->height = h;
	img->bpp = bpp;
	img->data = malloc(w * h * bpp);
	if (img->data == NULL) goto error;
	return img;

error:
	if (img != NULL) {
		free(img->data);
		free(img);
	}
	return NULL;
}

void imgSave(struct Image* img, const char* path)
{
	stbi_write_png(path, img->width, img->height, img->bpp, img->data, img->width*img->bpp);
}

void imgFree(struct Image* img)
{
	if (img == NULL) return;
	free(img->data);
	free(img);
}

void imgInverse(struct Image* img)
{
	int i;
	for (i = 0; i < img->width*img->height; i++)
		img->data[i] = 255-img->data[i];	
}

GLuint imgTexture(struct Image* img)
{
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, img->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, img->width, img->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, img->data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	return tex;	
}

int main()
{
	GLFWwindow* window;
	const GLFWvidmode* mode;
	double time;
	struct Image* img = NULL;
	struct Image* img2 = NULL;
	int64_t t0, t1;
	GLuint tex, tex2;

	// Load example image
	img = imgLoad("../example/flipper128.png", 1);
	if (img == NULL) {
		printf("Could not load image.\n");
		return -1;
	}
	imgInverse(img);

	// Build distance field and save it
	img2 = imgCreate(img->width, img->height, 1);
	if (img2 == NULL) {
		return -1;
	}

	t0 = getPerfTime();


//	sdfBuild(img2->data, img2->width, 2.0f, img->data, img->width, img->height, img->width);
	sdfCoverageToDistance(img2->data, img2->width, 2.0f, img->data, img->width, img->height, img->width);

	t1 = getPerfTime();

	imgSave(img2, "dist.png");

	printf("sdfBuild(%dx%d) %.1fms\n", img->width, img->height, deltaTimeUsec(t0, t1) / 1000.0f);

	if (!glfwInit())
		return -1;

	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	window = glfwCreateWindow(mode->width - 40, mode->height - 80, "Distance Transform", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);
	glfwMakeContextCurrent(window);

	tex = imgTexture(img);
	tex2 = imgTexture(img2);

	glEnable(GL_LINE_SMOOTH);

	time = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float o, s;
		int width, height;
		double t = glfwGetTime();
		double dt = t - time;
		if (dt > 0.5f) dt = 0.5f;
		time = t;

		glfwGetFramebufferSize(window, &width, &height);
		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_TEXTURE_2D);

		// Draw UI
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);


		s = height * (0.2f + (1.0f + sinf(time*0.5f))*0.5f);
		o = s*0.5f;

		// Draw orig texture using bilinear filtering.
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, tex);
		glColor4ub(255,255,255,255);

		glBegin(GL_QUADS);

		glTexCoord2f(0,0);
		glVertex2f(10+o,10);

		glTexCoord2f(1,0);
		glVertex2f(10+s+o,10);

		glTexCoord2f(1,1);
		glVertex2f(10+s+o,10+s);

		glTexCoord2f(0,1);
		glVertex2f(10+o,10+s);

		glEnd();


		// Draw distance texture using alha testing.
		glBindTexture(GL_TEXTURE_2D, tex2);
		glColor4ub(255,255,255,255);

		glDisable(GL_BLEND);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		glBegin(GL_QUADS);

		glTexCoord2f(0,0);
		glVertex2f(10,10);

		glTexCoord2f(1,0);
		glVertex2f(10+s,10);

		glTexCoord2f(1,1);
		glVertex2f(10+s,10+s);

		glTexCoord2f(0,1);
		glVertex2f(10,10+s);

		glEnd();

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glDisable(GL_ALPHA_TEST);

		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	imgFree(img);
	imgFree(img2);

	glfwTerminate();
	return 0;
}
