# YabbaColladaDoo
An example of loading a collada file that could be used as a starting point.

## About
This is a simple Collada loader that is really intended to be used to load a Collada file which would then be processed further depending on the needs, requirements, and format being used in say a game. I have included an example of initilizing a model directly from the XBot collada file and have only tested the initilization using said Collada file. So, more work would need to be done if you intend to use the same routine to initialize some other model.
Moreoever, the rendering setup and test depends on this model and in general will not work with other models.

The routine to call to load a collada file is ColladaFileLoad. It will load the Collada file and represent it as a unary tree. Each node in the tree has:

- A Tag which is the name of the node in the collada file
- An InnerText string which is the actual data associated with a given node
- An array of Attributes which are strings of Key/Value pairs
- A parent node (if it is not the root node)
- An array of children nodes

The number of attributes of a node is set to a compile time constant COLLADA_ATTRIBUTE_MAX_COUNT and the number of children is also set to a compile time constant of COLLADA_NODE_CHILDREN_MAX_COUNT. This wastes memory but again this program is intended to be used to load a Collada file which would then be processed to another format which would then be used in some application.

## Dependencies
GLFW and GLEW which facilitate rendering tests via OpenGL.

Visual Studio 2019 if you intend to use the build.bat file to build.

## Setup
\> git clone https://github.com/JustinEchols/YabbaColladaDoo.git

\> cd YabbaColladaDoo

\YabbaColladaDoo> misc\shell.bat

\YabbaColladaDoo> cd src

\YabbaColladaDoo\src> build.bat

## Run
\YabbaColladaDoo\src> ..\build\main.exe
