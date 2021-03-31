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

GLuint tex = 0;
GLuint tex2 = 0;
float imageAspect = 1.0f;

int alphaTest = 1;
float scale = 1.0f;
float radius = 2.0f;
float x = 0.0f;
float y = 0.0f;

const char* imageFilename = "../example/test.png";
//const char* imageFilename = "../example/flipper128.png";

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

int loadImage(const char* imageFile, float radius, float* imageAspect, GLuint* tex, GLuint* texSDF)
{
	int64_t t0, t1;
	struct Image* img = NULL;
	struct Image* img2 = NULL;

	// Load example image
	img = imgLoad(imageFile, 1);
	if (img == NULL) {
		printf("Could not load image.\n");
		return 0;
	}
	imgInverse(img);

	*imageAspect = img->height / (float) img->width;

	// Build distance field and save it
	img2 = imgCreate(img->width, img->height, 1);
	if (img2 == NULL) {
		return 0;
	}

	t0 = getPerfTime();

	sdfBuildDistanceField(img2->data, img2->width, radius, img->data, img->width, img->height, img->width);
//	sdfCoverageToDistanceField(img2->data, img2->width, img->data, img->width, img->height, img->width);

	t1 = getPerfTime();

	imgSave(img2, "dist.png");

	printf("sdfBuild(%dx%d) %.1fms\n", img->width, img->height, deltaTimeUsec(t0, t1) / 1000.0f);

	if (tex != 0) glDeleteTextures(1, tex);
	*tex = imgTexture(img);

	if (texSDF != 0) glDeleteTextures(1, texSDF);
	*texSDF = imgTexture(img2);

	imgFree(img);
	imgFree(img2);

	return 1;
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_HOME && action == GLFW_PRESS) {
		radius += 0.5f;
		loadImage(imageFilename, radius, &imageAspect, &tex, &tex2);
	}
	if (key == GLFW_KEY_END && action == GLFW_PRESS) {
		radius -= 0.5f;
		if (radius < 0.0) radius = 0.0;
		loadImage(imageFilename, radius, &imageAspect, &tex, &tex2);
	}

	if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS) {
		scale *= 1.1f;
	}
	if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS) {
		scale /= 1.1f;
	}

	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		x += 50.0f * scale;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		x -= 50.0f * scale;
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		y += 50.0f * scale;
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		y -= 50.0f * scale;
	}

	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		alphaTest = !alphaTest;
	}
}

int main(int argc, const char* argv[])
{
	GLFWwindow* window;
	const GLFWvidmode* mode;
	double time;

	if(argc > 1)
		imageFilename = argv[1];

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

	if (!loadImage(imageFilename, radius, &imageAspect, &tex, &tex2))
		return -1;

	glEnable(GL_LINE_SMOOTH);

	time = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float o, w, h;
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

		w = (width-40) * 0.5f * scale;
		h = w * imageAspect;
		o = w;

		// Draw orig texture using bilinear filtering.
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, tex);
		glColor4ub(255,255,255,255);

		glBegin(GL_QUADS);

		glTexCoord2f(0,0);
		glVertex2f(x+o,y);

		glTexCoord2f(1,0);
		glVertex2f(x+w+o,y);

		glTexCoord2f(1,1);
		glVertex2f(x+w+o,y+h);

		glTexCoord2f(0,1);
		glVertex2f(x+o,y+h);

		glEnd();


		// Draw distance texture using alha testing.
		glBindTexture(GL_TEXTURE_2D, tex2);
		glColor4ub(255,255,255,255);

		if (alphaTest) {
			glDisable(GL_BLEND);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.5f);
		}

		glBegin(GL_QUADS);

		glTexCoord2f(0,0);
		glVertex2f(x,y);

		glTexCoord2f(1,0);
		glVertex2f(x+w,y);

		glTexCoord2f(1,1);
		glVertex2f(x+w,y+h);

		glTexCoord2f(0,1);
		glVertex2f(x,y+h);

		glEnd();

		glDisable(GL_TEXTURE_2D);

		if (alphaTest) {
			glEnable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
		}

		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}
