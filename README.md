# YabbaColladaDoo
An example of loading a collada file that could be used as a starting point.

## About
This is a simple Collada loader that is really intended to be used to load a Collada file which would then be processed further depending on the needs of say a game. I have included an example of exporting the XBot model/animation into a different format and loading the new format. The example code will not work in general for any collada file since I only tested the initilization using a single model. So, more work would be required if you intend to use the same routines to initialize some other model.

The routine to call to load a collada file is ColladaFileLoad. It will load the Collada file and represent it as a unary tree. Each node in the tree has:

- A Tag which is the name of the node in the collada file
- An InnerText string which is the actual data associated with a given node
- An array of attributes which are strings of key/value pairs
- A parent node (if it is not the root node)
- An array of children nodes

The number of attributes of a node is set to a compile time constant COLLADA_ATTRIBUTE_MAX_COUNT and the number of children is also set to a compile time constant of COLLADA_NODE_CHILDREN_MAX_COUNT. This wastes memory but again this program is intended to be used to load a Collada file which would then be processed to another format to be used in some application.

## Dependencies
None

## Setup
\> git clone https://github.com/JustinEchols/YabbaColladaDoo.git

\> cd YabbaColladaDoo

\YabbaColladaDoo>cd src

\YabbaColladaDoo\src>build.bat

## Example Usage
The example program built from the source accepts two command line arguments.

- What you want to export (either -m for mesh, or -a for animation)
- Path to a collada file

For example, to export the XBot mesh use (assuming you are in the same directory as the exe)

main.exe -m ..\data\XBot.dae

and to export the idle animation

main.exe -a ..\data\XBot_IdleShiftWeight.dae



  
