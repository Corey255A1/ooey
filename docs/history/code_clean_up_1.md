# Code Clean Up 1
Date: 5-24-2026

## Summary
We've made it pretty far along with our two rendering platforms, however, now it is time to do some code clean up. The Wayland and X11 platforms have a nice clean header for the end user to include, however the cpp files are a mess of class definitions and file scope functions. 

- Ensure that variable names are explcit and not shortened.
- Update the documents to make sure they are up to date and reflect our current understandings

As I am diving more into what it takes to create a GUI library, I'm realizing that there are kind of two main elements (at least in my mind right now)
The central idea of a UI frame work is to abstract away the platform specific details (which are are making progress on) defining a set of core rendering and interaction tools, and then build the more complex controls from these abstractions.
1) Rendering things to the screen
    1) The core to the UI is drawing elements to the screen and providing an API to draw things to the screen. Strokes, Fills, Text, etc.
    2) As part of this there needs to be an abstraction for the different platforms for the rendering of things to the screen that then the other components build from.
    3) We are already kind of doing that, but building that abstraction needs to be cleaner, right now its just a singular IRenderTarget that might become unmanangable
2) Interacting with the user
    1) Whether its mouse, keyboard, touch screen, a socket protocol, grpc, serial port commands. It should be easy to build wrappers for these input providers to feed in interaction messages to the core UI library.
    2) We are kind of doing that with the notion of Pointers, however need to make sure the Keyboard inputs are be handled correctly and that we are providing the right abstractions.
    3) Platform specific events should be converted into our own interaction event system, and then our event system should be used to propgate to the control handlers.

As of now the architecture is something like this:
- Renderer
    - Platform Specific implementations
- Interaction
    - Device Specific implemtations
- MVVMC
    - Core UI arcitecture to tie the View to the Controller and drive User changes to the View Model to the Model, and reflect changes to the View Model to the View

