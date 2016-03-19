# OpenGL ES 2.0 Demo with Qt 5 #

I put this application together for a presentation on OpenGL programming.

## Known Issues and Oddities ##

 * Models loaded from PLY files are not rendered with an index buffer, even though that's the obvious way to do it.  This is mostly because I copied the code to translate the data from an old project.  It does keep the example a little simpler though, and it moves an extra level of indirection out of the code for building the arrow transforms.
 * There's a normal map texture for the chicken that I never got around to using.  Here's a tutorial: http://learnopengl.com/#!Advanced-Lighting/Normal-Mapping
 * I don't do any face or model sorting to handle transparency.  This is on purpose, because I wanted to show what happens when you don't draw transparent triangles from back to front.  Also, I didn't want to take the time.  This does mean that the grid and arrows overlap incorrectly, but it's subtle enough that I'm satisfied with it for now.
