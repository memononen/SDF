
local action = _ACTION or ""

solution "distancefield"
	location ( "build" )
	configurations { "Debug", "Release" }
	platforms {"native", "x64", "x32"}
  
	project "example"
		kind "ConsoleApp"
		language "C"
		files { "example/example.c" }
		includedirs { "src", "example" }
		targetdir("build")
	 
		configuration { "linux" }
			 linkoptions { "`pkg-config --libs glfw3`" }
			 links { "GL", "GLU", "m" }

		configuration { "windows" }
			 links { "glfw3", "gdi32", "winmm", "user32", "glu32","opengl32" }

		configuration { "macosx" }
			links { "glfw3" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}    
