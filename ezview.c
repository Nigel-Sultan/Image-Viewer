#define GLFW_INCLUDE_ES2 1
#define GLFW_DLL 1
#define MAX_COLOR 255 //max color definition for later use
#define GL_GLEXT_PROTOTYPES

#include <GLFW/glfw3.h>
#include <GLES2/gl2.h>

#include "linmath.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


typedef struct Pixel
{
    double r;    //create a pixel struct like discussed in class
    double g;
    double b;
}Pixel;

typedef struct Image
{   //image struct that keeps track of the data in the input file
    int width;
    int height;
    int color;
    unsigned char *data;
} Image;

typedef struct Vertex 
{
	float position[2];
	float TexCoord[2];
} Vertex;

Vertex Vertexes[] = 
{
	{ { -1, 1}, {0, 0} },
	{ { 1, 1}, {0.99999 , 0}},
	{ { -1, -1}, {0, 0.99999}},
	{ { 1, 1}, {0.99999, 0}},
	{{1, -1}, {0.99999, 0.99999}},
	{{-1, -1}, {0, 0.99999}}
};

char* vertex_shader_src =
"uniform mat4 MVP;\n"
"attribute vec2 vPos;\n"
"attribute vec2 TexCoordIn;\n"
"varying vec2 TexCoordOut;\n"
"\n"
"void main(void) {\n"
"    gl_Position = Position* vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}";


char* fragment_shader_src =
"varying vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"\n"
"void main(void) {\n"
"    gl_FragColor = texture2D(Texture, DestinationTexcoord);\n"
"}";

Image *buffer;

static void error_callback(int, const char*);
static void key_callback(GLFWwindow*, int, int, int, int);
Image read_data(char*);
int write_data(int, FILE*);
int write_image(int, char*);
void glCompileShaderOrDie(GLuint);

float scaleTo[2] = { 1.0, 1.0 };
float shearTo[2] = { 0.0, 0.0 };
float translationTo[2] = { 0.0, 0.0 };
float rotationTo = 0;

Image read_data(char *input_file){
	buffer = (Image*)malloc(sizeof(Image));		//allocates memory for buffer
	
	FILE* ifp = fopen(input_file, "rb");		//opens file for reading
	
	if (ifp == NULL) 	//checks if file pointer can be made 'if file can be opened'
	{
        fprintf(stderr, "Error: Could not open file \"%s\"\n", input_file);		//prints error and closes file
        exit(1);
	}
	
	int current = fgetc(ifp);	//sets variable equal to first character in file
	if(current != 'P')
	{
        fprintf(stderr, "Error: This is not a PPM file!\n");    //checks if file is a ppm file
        fclose(ifp);
        exit(1);
	}
	
	current = fgetc(ifp);
	
	int ppm_type = current;
    if((ppm_type != '3') & (ppm_type != '6'))
	{
        fprintf(stderr, "Error: Only P3/P6 files allowed!\n");
        fclose(ifp);
        exit(1);
	}
	
	while(current != '\n')
	{     //skips newlines
        current = fgetc(ifp);
    }
    
	current = fgetc(ifp);
	
	while(current == '#')
	{      //skips comments
        while(current != '\n')
		{
            current = fgetc(ifp);
        }
        current = fgetc(ifp);
    }
	
	ungetc(current, ifp);

	int dimensions = fscanf(ifp, "%d %d", &buffer->width, &buffer->height);     //saves image width and height in buffer
    if(dimensions != 2)
	{                                                        //checks if dimensions are correct amount
        fprintf(stderr, "Error: Not correct dimensions of width and height!\n");
        fclose(ifp);
        exit(1);
    }

    int max_color = fscanf(ifp, "%d", &buffer->color);      //checks if max color value is in correct range
    if((max_color != 1) | (buffer->color > MAX_COLOR))
	{
        fprintf(stderr, "Error: Invalid color channel value\n");
        fclose(ifp);
        exit(1);
	}
	
	buffer->data = (unsigned char*)malloc((buffer->width) * (buffer->height) * (sizeof(Pixel)));    //allocates memory for the buffer data
    if(buffer == NULL)
	{
        fprintf(stderr, "Error: memory allocation unsuccessful\n");
        fclose(ifp);
        exit(1);
	}
	
	if(ppm_type == '3')
	{        //if the image is a p3 file this for loop writes the data into the pixels struct for rgb like discussed in class
        int i, j;
        for(i=0; i<buffer->height; i++)
		{
            for(j=0; j<buffer->width; j++)
			{
                Pixel *pixels = (Pixel*)malloc(sizeof(Pixel));
                fscanf(ifp, "%hhd %hhd %hhd", &pixels->r, &pixels->g, &pixels->b);
                buffer->data[i*buffer->width * 3 + j * 3] = pixels->r;
                buffer->data[i*buffer->width * 3 + j * 3 + 1] = pixels->g;
                buffer->data[i*buffer->width * 3 + j * 3 + 2] = pixels->b;
            }
        }
    }
    else
	{
        size_t s = fread(buffer->data, sizeof(Pixel), buffer->width*buffer->height, ifp);   //saves p6 data into buffer
        if(s < buffer->width*buffer->height)
		{
            fprintf(stderr, "Error: Can't convert!");
            fclose(ifp);
            exit(1);
        }
	}
	
	fclose(ifp);
	return *buffer;
}


int write_image(int int_format, char *output_file)
{ 	
	//function that writes data to new file
    FILE *ofp = fopen(output_file, "wb");   //opens output file for writing

    char *extension = "# output.ppm";   //adds extension to output file to make it readable
    int width = buffer->width;      //saves dimensions from buffer into variables
	int height = buffer->height;
	
	if (ofp == NULL) 
	{
		fprintf(stderr, "Error: open the file unscuccessfully. \n");
		fclose(ofp);
		return (1);
	}
	
	fprintf(ofp, "P%i\n%s\n%d %d\n%d\n", int_format, extension, width, height, MAX_COLOR); //prints correct header for new output file
	
	write_data(int_format, ofp);
	
	fclose(ofp);
	
	return (0);
}

int write_data(int int_format, FILE *output_filename)
{
	if(int_format == 6)
	{        //writes data if being converted to P6
        fwrite(buffer->data, sizeof(Pixel), buffer->width*buffer->height, output_filename);
        printf("The file converted successfully!\n");
    }
    else if(int_format == 3)
	{   //writes data if being converted to P3
        int i, j;
        for(i=0; i<buffer->height; i++)
		{
            for(j=0; j<buffer->width; j++)
			{
                fprintf(output_filename, "%d %d %d ", buffer->data[i*buffer->width*3+j*3], buffer->data[i*buffer->width+j*3+1], buffer->data[i*buffer->width*3+2]);
            }
            fprintf(output_filename, "\n");
        }
        printf("Image converted successfully!\n");
    }
    return(0);
}


static void error_callback(int error, const char* description) 
{
	fprintf(stderr, "Error: %s\n", description);
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) 
	{
		if (key == GLFW_KEY_R)
		{
			scaleTo[0] = 1;
			scaleTo[1] = 1;
			
			rotationTo = 0;
			
			translationTo[0] = 0;
			translationTo[1] = 0;
			
			shearTo[0] = 0;
			shearTo[1] = 0;
		}

		if (key == GLFW_KEY_E) 
		{
			glfwWindowShouldClose(window);
		}
	
		if (key == GLFW_KEY_W) 
		{
			scaleTo[0] *= 2;
			scaleTo[1] *= 2;
		}

		if (key == GLFW_KEY_S)
		{
			scaleTo[0] /= 2;
			scaleTo[1] /= 2;	
		}
		
		if (key == GLFW_KEY_D) 
		{
			rotationTo += 0.5;
		}
		
		if (key == GLFW_KEY_A) 
		{
			rotationTo -= 0.5;
		}

		if (key == GLFW_KEY_RIGHT) 
		{
			scaleTo[0] *= 2;
		}

		if (key == GLFW_KEY_LEFT) 
		{
			scaleTo[0] /= 2;
		}

		if (key == GLFW_KEY_UP)
		{
			scaleTo[1] *= 2;
		}

		if (key == GLFW_KEY_DOWN) 
		{
			scaleTo[1] /= 2;
		}
	
		if (key == GLFW_KEY_H) 
		{
			translationTo[0] += 1;
		}

		if (key == GLFW_KEY_F) 
		{
			translationTo[0] -= 1;
		}

		if (key == GLFW_KEY_T) 
		{
			translationTo[1] += 1;
		}

		if (key == GLFW_KEY_G) 
		{
			translationTo[1] -= 1;
		}

		if (key == GLFW_KEY_L) 
		{
			shearTo[0] += 1;
		}

		if (key == GLFW_KEY_J) 
		{
			shearTo[0] -= 1;
		}

		if (key == GLFW_KEY_I) 
		{
			shearTo[1] += 1;
		}

		if (key == GLFW_KEY_K) 
		{
			shearTo[1] -= 1;
		}
	}
}

void glCompileShaderOrDie(GLuint shader)
{
	GLint compiled;
	glCompileShader(shader);
	glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader,
			GL_INFO_LOG_LENGTH,
			&infoLen);
		char* info = malloc(infoLen + 1);
		GLint done;
		glGetShaderInfoLog(shader, infoLen, &done, info);
		printf("Unable to compile shader: %s\n", info);
		exit(1);
	}
}

int main(int argc, char *argv[]) 
{
	if(argc != 2)
	{
        fprintf(stderr, "Error: Too many arguments!\n");
        return(1);
	}

	char *input_file = argv[1];
	read_data(input_file);

	GLFWwindow* window;
	GLuint program_id, vertex_shader, fragment_shader;
	GLuint vertex_buffer;
	GLint mvp_location, vpos_location, texcoord_location, texture_location;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		return -1;

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(650, 650, "Image Viewer", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(1);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	program_id = glCreateProgram();
	
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertexes), Vertexes, GL_STATIC_DRAW);


	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
	glCompileShaderOrDie(vertex_shader);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
	glCompileShaderOrDie(fragment_shader);

	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);

	mvp_location = glGetUniformLocation(program_id, "MVP");
	assert(mvp_location != -1);
	
	vpos_location = glGetAttribLocation(program_id, "vPos");
	assert(vpos_location != -1);

	texcoord_location = glGetAttribLocation(program_id, "TexCoordIn");
	assert(texcoord_location != -1);

	texture_location = glGetUniformLocation(program_id, "Texture");
	assert(texture_location != -1);

	glEnableVertexAttribArray(vpos_location);
	glEnableVertexAttribArray(texcoord_location);

	glVertexAttribPointer(vpos_location,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		0);

	glVertexAttribPointer(texcoord_location,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(Vertex),
		(GLvoid*)(sizeof(float) * 2));


	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer->width, buffer->height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer->data);

	


	while (!glfwWindowShouldClose(window)) {
		int buffer_width, buffer_height;
		glfwGetFramebufferSize(window, &buffer_width, &buffer_height);

		float ratio;
		mat4x4 s, sh, t, r, p, ssh, ssht, mvp;

		ratio = buffer_width / (float)buffer_height;

		glViewport(0, 0, buffer_width, buffer_height);
		glClear(GL_COLOR_BUFFER_BIT);


		mat4x4_identity(s);

		s[0][0] = s[0][0] * scaleTo[0];
		s[1][1] = s[1][1] * scaleTo[1];
		
		sh[0][1] = shearTo[0];
		sh[1][0] = shearTo[1];
		
		mat4x4_identity(t);
		mat4x4_translate(t, translationTo[0], translationTo[1], 0);

		mat4x4_identity(r);
		mat4x4_rotate_Z(r, r, rotationTo);


		mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_mul(ssh, s, sh);
		mat4x4_mul(ssht, ssh, t);
		mat4x4_mul(mvp, ssht, r);

		
		
		glUseProgram(program_id);
		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
