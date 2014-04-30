SDF - Build Signed Distance Field from Antialiased Image
========================================================

Sweep-and-update Euclidean distance transform of an antialised image for contour textures.

The code is based on [http://contourtextures.wikidot.com/](edtaa3func.c) by Stefan Gustavson and improves the original in terms of memory usage and execution time.

The algorithms first traverse the image and uses gradient direction and the edge function from edtaa3 to find an approximated point on the contour of the input image. After this pass the distance at the edge pixels are known, and the code proceeds to update the rest of the distance field using sweep-and-update until the distance feild convergences (or max passes run).

The additional memory required by the code is 3 floats (px,py,distance) per pixel, compared to 5 doubles (or floats) of edtaa3. The sweep-and-update is
done using squared distances and contour points calculated only in the first pass. This is greatly reduces the amount of computation (especially suqare roots) per distance update.

The code procudes comparable, but probably not as accurate distance fields as the original code.

The code is intended to be used to calculate distance fields for [http://contourtextures.wikidot.com/](contour texturing).

**Usage
```C
int sdfBuild(unsigned char* out, int outstride, float maxdist,
			 const unsigned char* img, int width, int height, int stride);
```
The output distance field is encoded as bytes, where 0 = maxdist (outside) and 255 = -maxdist (inside). Input and output can be the same buffer.
* /out/ - Output of the distance transform, one byte per pixel.
* /outstride/ - Bytes per row on output image. 
* /maxdist/ - The extents of the output distance range in pixels.
* /img/ - Input image, one byte per pixel.
* /width/ - Width if the image. 
* /height/ - Height if the image. 
* /stride/ - Bytes per row on input image. 

White (255) pixels are treated as object pixels, zero pixels are treated as background. An attempt is made to treat antialiased edges correctly. The input image must have pixels in the range [0,255], and the antialiased image should be a box-filter sampling of the ideal, crisp edge. If the antialias region is more than 1 pixel wide, the result from this transform will be inaccurate. Pixels at image border are not calculated and are set to 0.
(Explanation borrowed from the original eedtaa3func.c)

```C
void sdfBuildNoAlloc(unsigned char* out, int outstride, float maxdist,
					 const unsigned char* img, int width, int height, int stride,
					 unsigned char* temp);
```

Same as distXform, but does not allocate any memory. The 'temp' array should be enough to fit width * height * sizeof(float) bytes.

**License
MIT License
