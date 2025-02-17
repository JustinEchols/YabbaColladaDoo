# YabbaColladaDoo
An example of loading a collada file that could be used as a starting point.

## About
This is a simple Collada loader that is really intended to be used to load a Collada file which would then be processed further depending on the needs, requirements, and format being used in say a game. I have included an example of loading and converting model data as well as animation data. More testing and work is required to ensure that different models load properly. Moreover, the rendering setup and tests are quite minimal. 

The routine to call to load a collada file is ColladaFileLoad. It will load the Collada file and represent it as a unary tree. Each node in the tree has:

- A Tag which is the name of the node in the collada file
- An InnerText string which is the actual data associated with a given node
- An array of Attributes which are strings of Key/Value pairs
- A parent node (if it is not the root node)
- An array of children nodes

The number of attributes of a node is set to a compile time constant COLLADA_ATTRIBUTE_MAX_COUNT and the number of children is also set to a compile time constant of COLLADA_NODE_CHILDREN_MAX_COUNT. This wastes memory but again this program is intended to be used to load a Collada file which would then be processed to another format to be used in some other application.

## Dependencies
C runtime library.

GLFW and GLEW which facilitate rendering tests via OpenGL.

stbi_image.h for image loading.

Visual Studio 2019 if you intend to use the build.bat file to build.

## Setup
\> git clone https://github.com/JustinEchols/YabbaColladaDoo.git

\> cd YabbaColladaDoo

\YabbaColladaDoo> misc\shell.bat

\YabbaColladaDoo> cd src

\YabbaColladaDoo\src> build.bat

## Run
If running the rendering test, the application needs to be ran in a debugger and the working directory must be set to the data directory. If converting the Collada file only, the application can be ran from a terminal.
