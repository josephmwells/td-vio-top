/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "VioTOP.h"

#include <assert.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <string.h>
#endif
#include <cstdio>

using namespace VioApi;

#if _M_X64
#pragma comment(lib,"lib/x64/VioApi")
#else
#pragma comment(lib,"lib/x86/VioApi")
#endif

static const char *vertexShader = "#version 330\n\
uniform mat4 uModelView; \
in vec3 P; \
void main() { \
	gl_Position = vec4(P, 1) * uModelView; \
}";

static const char *fragmentShader = "#version 330\n\
uniform vec4 uColor; \
out vec4 finalColor; \
void main() { \
	finalColor = uColor; \
}";

static const char *uniformError = "A uniform location could not be found.";

// Setup error handling for Ventuz VIO functions
#define VERR(x) vErr((x),__FILE__,__LINE__)

bool vErr(vError err, const char* file, int line)
{
	if (err == VE_Ok)
		return true;
	//DPrintF("%s(%d): %s\n", file, line, vGetErrorString(err));
	return false;
}

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{
DLLEXPORT
void
FillTOPPluginInfo(TOP_PluginInfo *info)
{
	// This must always be set to this constant
	info->apiVersion = TOPCPlusPlusAPIVersion;

	// Change this to change the executeMode behavior of this plugin.
	info->executeMode = TOP_ExecuteMode::OpenGL_FBO;

	// The opType is the unique name for this TOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Openglsample");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("OpenGL Sample");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("OGL");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Author Name");
	info->customOPInfo.authorEmail->setString("email@email.com");

	// This TOP works with 0 inputs
	info->customOPInfo.minInputs = 0;
	info->customOPInfo.maxInputs = 0;

}

DLLEXPORT
TOP_CPlusPlusBase*
CreateTOPInstance(const OP_NodeInfo* info, TOP_Context *context)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per TOP that is using the .dll

	// Note we can't do any OpenGL work during instantiation

	return new VioTOP(info, context);
}

DLLEXPORT
void
DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the TOP using that instance is deleted, or
	// if the TOP loads a different DLL

	// We do some OpenGL teardown on destruction, so ask the TOP_Context
	// to set up our OpenGL context
	context->beginGLCommands();

	delete (VioTOP*)instance;

	context->endGLCommands();
}

};


VioTOP::VioTOP(const OP_NodeInfo* info, TOP_Context *context)
: myNodeInfo(info), myExecuteCount(0), myRotation(0.0), myError(nullptr),
	myProgram(), myDidSetup(false), myModelViewUniform(-1), myColorUniform(-1),
	VioHandle(0)
{

#ifdef _WIN32
	// GLEW is global static function pointers, only needs to be inited once,
	// and only on Windows.
	static bool needGLEWInit = true;
	if (needGLEWInit)
	{
		needGLEWInit = false;
		context->beginGLCommands();
		// Setup all our GL extensions using GLEW
		glewInit();
		context->endGLCommands();
	}
#endif

	// Initialize VIO Stream and set parameters

	// Initialize for OPEN_GL
	//VERR(vInit(0, 0, 0));
	vInit(0, 0, 0);

	memset(&OpenPara, 0, sizeof(OpenPara));
	OpenPara.Channel = 0;
	OpenPara.Mode = VM_ToVentuz;
	OpenPara.Transfer = VTM_OpenGl;
	OpenPara.Color = VCP_RGBA_8;
	OpenPara.Depth = VDP_Off;
	OpenPara.SizeX = 256;
	OpenPara.SizeY = 256;

	// If you wanted to do other GL initialization inside this constructor, you could
	// uncomment these lines and do the work between the begin/end
	//
	//context->beginGLCommands();
	// Custom GL initialization here
	//context->endGLCommands();
}

VioTOP::~VioTOP()
{
	vClose(VioHandle);
	VioHandle = 0;
	vExit();
}

void
VioTOP::getGeneralInfo(TOP_GeneralInfo* ginfo, const OP_Inputs *inputs, void* reserved1) 
{
	// Setting cookEveryFrame to true causes the TOP to cook every frame even
	// if none of its inputs/parameters are changing. Set it to false if it
	// only needs to cook when inputs/parameters change.
	ginfo->cookEveryFrame = true;
}

bool
VioTOP::getOutputFormat(TOP_OutputFormat* format, const OP_Inputs *inputs, void* reserved1)
{
	// In this function we could assign variable values to 'format' to specify
	// the pixel format/resolution etc that we want to output to.
	// If we did that, we'd want to return true to tell the TOP to use the settings we've
	// specified.
	// In this example we'll return false and use the TOP's settings
	format->width = 256;
	format->height = 256;
	format->aspectX = 1;
	format->aspectY = 1;
	return true;
}


void
VioTOP::execute(TOP_OutputFormatSpecs* outputFormat ,
							const OP_Inputs* inputs,
							TOP_Context* context,
							void* reserved1)
{
	myError = nullptr;
	myExecuteCount++;

	// These functions must be called before
	// beginGLCommands()/endGLCommands() block
	double speed = inputs->getParDouble("Speed");

	double color1[3];
	double color2[3];

	inputs->getParDouble3("Color1", color1[0], color1[1], color1[2]);
	inputs->getParDouble3("Color2", color2[0], color2[1], color2[2]);

	myRotation += speed;

	int width = outputFormat->width;
	int height = outputFormat->height;

	float ratio = static_cast<float>(height) / static_cast<float>(width);

	Matrix view;
	view[0] = ratio;

	//context->getShareRenderContext();

	//context->beginGLCommands();

	if (!VioHandle)
	{
		if (!VERR(vOpen(&OpenPara, &VioHandle)))
			return;
	}

	vFrame frame;
	vError error = vLockFrame(VioHandle, &frame);
	if (!VERR(error))
	{
		if (error == VE_ConnectionBroken)
			VioHandle = 0;
		return;
	}
	
	setupGL();

	if (!myError)
	{
		

		
		
		glViewport(0, 0, width, height);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(myProgram.getName());

		// Draw the square

		glUniform4f(myColorUniform, static_cast<GLfloat>(color1[0]), static_cast<GLfloat>(color1[1]), static_cast<GLfloat>(color1[2]), 1.0f);

		mySquare.setTranslate(0.5f, 0.5f);
		mySquare.setRotation(static_cast<GLfloat>(myRotation));

		Matrix model = mySquare.getMatrix();
		glUniformMatrix4fv(myModelViewUniform, 1, GL_FALSE, (model * view).matrix);

		mySquare.bindVAO();

		glDrawArrays(GL_TRIANGLES, 0, mySquare.getElementCount() / 3);

		// Draw the chevron

		glUniform4f(myColorUniform, static_cast<GLfloat>(color2[0]), static_cast<GLfloat>(color2[1]), static_cast<GLfloat>(color2[2]), 1.0f);

		myChevron.setScale(0.8f, 0.8f);
		myChevron.setTranslate(-0.5, -0.5);
		myChevron.setRotation(static_cast<GLfloat>(myRotation));

		model = myChevron.getMatrix();
		glUniformMatrix4fv(myModelViewUniform, 1, GL_FALSE, (model * view).matrix);

		myChevron.bindVAO();

		glDrawArrays(GL_TRIANGLES, 0, myChevron.getElementCount() / 3);

		// Tidy up

		glBindVertexArray(0);
		glUseProgram(0);

		//VERR(vUnlockFrame(VioHandle));
	}

	//context->endGLCommands();
}

int32_t
VioTOP::getNumInfoCHOPChans(void * reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the TOP. In this example we are just going to send one channel.
	return 2;
}

void
VioTOP::getInfoCHOPChan(int32_t index, OP_InfoCHOPChan* chan, void * reserved1)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.

	if (index == 0)
	{
		chan->name->setString("executeCount");
		chan->value = (float)myExecuteCount;
	}

	if (index == 1)
	{
		chan->name->setString("rotation");
		chan->value = (float)myRotation;
	}
}

bool		
VioTOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 2;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
VioTOP::getInfoDATEntries(int32_t index,
										int32_t nEntries,
										OP_InfoDATEntries* entries,
										void* reserved1)
{
	char tempBuffer[4096];

	if (index == 0)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "executeCount");
#else // macOS
		strlcpy(tempBuffer, "executeCount", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%d", myExecuteCount);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%d", myExecuteCount);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 1)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "rotation");
#else // macOS
		strlcpy(tempBuffer, "rotation", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%g", myRotation);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myRotation);
#endif
		entries->values[1]->setString(tempBuffer);
	}
}

void
VioTOP::getErrorString(OP_String *error, void* reserved1)
{
	error->setString(myError);
}

void
VioTOP::setupParameters(OP_ParameterManager* manager, void* reserved1)
{
	// color 1
	{
		OP_NumericParameter	np;

		np.name = "Color1";
		np.label = "Color 1";

		np.defaultValues[0] = 1.0;
		np.defaultValues[1] = 0.5;
		np.defaultValues[2] = 0.8;

		for (int i=0; i<3; i++)
		{
			np.minValues[i] = 0.0;
			np.maxValues[i] = 1.0;
			np.minSliders[i] = 0.0;
			np.maxSliders[i] = 1.0;
			np.clampMins[i] = true;
			np.clampMaxes[i] = true;
		}
		
		OP_ParAppendResult res = manager->appendRGB(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// color 2
	{
		OP_NumericParameter	np;

		np.name = "Color2";
		np.label = "Color 2";

		np.defaultValues[0] = 1.0;
		np.defaultValues[1] = 1.0;
		np.defaultValues[2] = 0.25;

		for (int i=0; i<3; i++)
		{
			np.minValues[i] = 0.0;
			np.maxValues[i] = 1.0;
			np.minSliders[i] = 0.0;
			np.maxSliders[i] = 1.0;
			np.clampMins[i] = true;
			np.clampMaxes[i] = true;
		}
		
		OP_ParAppendResult res = manager->appendRGB(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// speed
	{
		OP_NumericParameter	np;

		np.name = "Speed";
		np.label = "Speed";
		np.defaultValues[0] = 1.0;
		np.minSliders[0] = -10.0;
		np.maxSliders[0] =  10.0;
		
		OP_ParAppendResult res = manager->appendFloat(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// pulse
	{
		OP_NumericParameter	np;

		np.name = "Reset";
		np.label = "Reset";
		
		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

}

void
VioTOP::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Reset"))
	{
		myRotation = 0.0;
	}
}

void VioTOP::setupGL()
{
	if (myDidSetup == false)
	{
		myError = myProgram.build(vertexShader, fragmentShader);

		// If an error occurred creating myProgram, we can't proceed
		if (myError == nullptr)
		{
			GLint vertAttribLocation = glGetAttribLocation(myProgram.getName(), "P");
			myModelViewUniform = glGetUniformLocation(myProgram.getName(), "uModelView");
			myColorUniform = glGetUniformLocation(myProgram.getName(), "uColor");

			if (vertAttribLocation == -1 || myModelViewUniform == -1 || myColorUniform == -1)
			{
				myError = uniformError;
			}

			// Set up our two shapes
			GLfloat square[] = {
				-0.5, -0.5, 1.0,
				0.5, -0.5, 1.0,
				-0.5,  0.5, 1.0,

				0.5, -0.5, 1.0,
				0.5,  0.5, 1.0,
				-0.5,  0.5, 1.0
			};

			mySquare.setVertices(square, 2 * 9);
			mySquare.setup(vertAttribLocation);

			GLfloat chevron[] = {
				-1.0, -1.0,  1.0,
				-0.5,  0.0,  1.0,
				0.0, -1.0,  1.0,

				-0.5,  0.0,  1.0,
				0.5,  0.0,  1.0,
				0.0, -1.0,  1.0,

				0.0,  1.0,  1.0,
				0.5,  0.0,  1.0,
				-0.5,  0.0,  1.0,

				-1.0,  1.0,  1.0,
				0.0,  1.0,  1.0,
				-0.5,  0.0,  1.0
			};

			myChevron.setVertices(chevron, 4 * 9);
			myChevron.setup(vertAttribLocation);
		}

		myDidSetup = true;
	}
}
