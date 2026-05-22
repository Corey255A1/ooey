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

## Building and Running

To compile and run the engine, especially the example that opens a native window, you'll need a few dependencies and a modern C++ compiler. 

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

### Running the Example
Once compiled, you can run the `hello_ooey` example, which opens an 800x600 window with a red square rendered using OpenGL:
```bash
./build/examples/hello_ooey
```

### Running Tests
To run the automated GoogleTest suite:
```bash
cd build
ctest --output-on-failure
```

# Task List
- Project Skeleton and Build System Setup
    - Initialize CMake project (CMakeLists.txt) enforcing C++20.
    - Create foundational directory structure (`src`, `include`, `tests`).
- Core Architecture & Abstractions
    - Define abstract interfaces for `RenderTarget` (Window, Memory, File).
    - Define abstract interfaces for `WindowBackend` (Wayland, X11, Windows, macOS).
    - Create basic data structures for rendering (e.g., `Color`, `Rect`, `Point`).
- Initial Linux/Chromebook Implementation
    - Implement a basic Window backend using Wayland or X11.
    - Implement a basic RenderTarget for the Window (potentially using OpenGL or Vulkan).
- Expanded Rendering Capabilities
    - Implement an in-memory RenderTarget for CPU-side rendering.
    - Implement a File RenderTarget (e.g., outputting to a raw image file).
- Media and Asset Integration (Future)
    - Integrate `libpng`/`libjpeg` for image loading and display.
    - Explore `alsa`/`pulseaudio` for sound and `ffmpeg` for video playback.
