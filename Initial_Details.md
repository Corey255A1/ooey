# OOEY
I've been wanting to get on the AI Coding hype train for a while, but I haven't had a solid project idea that was complex enough, but also something that I could still reasonably understand. 

The idea is to create a C++ GUI engine that can render to a window, the whole screen, to a block of memory, to a file, etc. The core idea, is that the result of the render can easily be something in a graphics card frame buffer, or a cpu memory block, or rendered out to a file, or to a webbrowser canvas (with something like emscripten). Long term, I would want this to cross compile to any platform.

The code should use modern C++ 20 where it makes the most sense. I don't however want to go down the rabbit hole of modules. I want to stick with include files.

The system should use CMake to manage the project structure.

To make this have some cool features, I'll need to include some additional optional libraries depending on what mode you want to use.
Things like
- Wayland, X11 libraries if you are trying to open up windows
- OpenGL, Vulkan, DirectX if you are trying to leverage graphics card capabilities
- libpng, libjpeg, etc if you want to display images
- alsa, pulse audio, etc if you want to play sounds
- ffmpeg, gstreamer, if you want to play videos

Long term dependency would be on the Nick Lohmann Json library for View code, but for now it will all be pure C++

Ooey should be an easy to use library with an inuitive API. 

Long term goal to have a declarative UI definition file that allows you to specifify the states of the various UI controls based purely on state of the viewmodel.  

The API and code should be strict Model-View-ViewModel-Controller. Meaning that it will be a very opinionated API. Model->ViewModel->View. Where the Controller processes user input from the keyboard, touchscreen, microphone, AI model, etc, has an understanding of how the View is composed to know when a cursor is hovering over a button, or a keyboard, virtual keyboard, key press was typed and the user focus was in a text field, to then forward that key event to the viewmodel for processing.

Lets take a basic case as an example:
```json
Model: {
    IsOperationValid: true
}

ViewModel: {
    OperationAction: ActionModel()
    {
        CanExecute: Model.IsOperationValid
        OnExecute: {

        }
    }
}

View: {
    Button:
    {
        Model: ViewModel.OperationAction
    } 
    
}

```

Something like that is where my head is at. However, this will all be raw C++ api to accomplish this.

## More thoughts
ok, this is looking good for a prototype. Now here are some additional thoughts and ideas on the architecture. Having a class or interface that defines a bunch of draw functions like draw_rect or draw_line is not exentisble. THere should be a base level primative object Drawable (or something) that can then build built up with lines, rectangles, circles,, text These drawables are then added to the draw surface and when their draws are called, at the lowest level they will draw with the x11/ wayland/ opengl/ etc calls. But at the highest level, when I am using the API to draw out a UI, I should be using encapsulated objects, like button, rectangle etc, that  are built upon primatives, that are then built upon even lower primatives that are the platform indepdent primatives... THen somethign like a button would be composed of stack of objects that get rendered in an order to draw the resulting button.

Additionally, for user input, I want the concept to be that there exists list of pointer points that can track any number of active pointer objects. A pointer object can resprent tracking the motion of a mouse pointer over the UI, it can represnet multitouch inputs over the UI, or it can represent a pointer driver that is being controlled by the arrow keys to move a pointer around the UI. Or a pointer could be driven by some other external piece of hardware from a serial port, etc. The pointer object doesn't care what is driving it. And the same for key presses, they can be coming from computer keyboard, input from another process, etc. It would just be a matter of having the plugin loaded to insert that data. The idea is that I could use this library on linux, windows, embedded, raspberry pi, all sorts of things

Lets define and document how to make these objects interactive. With our base level Pointer notion, there needs to be a way to track when a pointer is over top an area of objects. A typical UI framework will bubble these events from the top object to the bottom object .. as in if there is a button on top, that will process the ponter event first, if it doesn't handle it, it passes it down to the next object in the heirarchy. There needs to be a way to define a heirarchy of objects to handle these types of interactions by a pointer object. But also, lets say I have a device that is not touch screen, doesn't have a mouse, and just has a keyboard, our API should be able to still be able to handle those input events based on the current visual state, That is where a controller will come in handy

All GUI frameworks have a way to display and edit text. We should be able to display the typical single line or multiline texts as well as having editable single line or multiline texts. I want it to be able to use system fonts, or just default to a built in text renderer for basic english texts. Fonts should be scaleable, colorable, bold, italic.

## Library Architecture

OOEY is divided into two separate libraries to maximize portability and keep core platform dependencies separated from the UI logic:

1. **`ooey`** (Graphics & Platform Core)
   - Encapsulates OS surface/window creation (X11, Wayland, Linux Framebuffer, Emscripten).
   - Provides graphics API wrappers (`GlRenderTarget` for OpenGL-based systems, `SoftwareRenderTarget` for raw framebuffer/memory-block rasterization).
   - Manages unified input manager and pointer tracking.
   - All classes in this library reside under the `ooey` namespace.

2. **`gooey`** (UI Toolkit & MVVMC)
   - Dependent on the `ooey` library.
   - Implements layout architecture (`Application`, `View`), interactive components (`Button`, `Label`, `TextBox`, `ListControl`), MVVMC bindings (`Controller`, `NavigationCoordinator`, `Property`), and primitive shape drawing.
   - All classes in this library reside under the `gooey` namespace.

## Building and Running

To compile and run the engine, especially the examples that open native windows, you'll need a few dependencies and a modern C++ compiler. 

### Dependencies (Debian/Ubuntu/ChromeOS Crostini)
You need CMake, a C++20 compatible compiler, and the X11/OpenGL development libraries:
```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential libx11-dev libgl1-mesa-dev libglx-dev
```

### Compiling the Project
We use CMake to build out-of-source:
```bash
# 1. Generate the build files
cmake -B build -S .

# 2. Compile the project
cmake --build build -j$(nproc)
```

This compiles:
- `libooey.a`: The core graphics/platform library.
- `libgooey.a`: The UI toolkit library.

### Running Examples
Once compiled, you can run various examples from the `build/examples` directory:
```bash
# Run X11 OpenGL hello world
./build/examples/hello_ooey

# Run the MVVM-C example
./build/examples/hello_ooey_mvvmc

# Run the Wizard application
./build/examples/hello_wizard
```

### Running Tests
To run the automated GoogleTest suite:
```bash
./build/tests/ooey_tests
```

