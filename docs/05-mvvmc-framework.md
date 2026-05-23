# Phase 5: Strict MVVM-C Framework (Pending)

## Goal
Implement the highly opinionated Model-View-ViewModel-Controller architecture described in the project goals, entirely in pure C++.

## Action Items

1. **Controller Design (Pending)**
   - Implement the `Controller` layer responsible for processing raw system events (keyboard, mouse, touch).
   - It needs to understand the View's layout to determine focus (e.g., hover states, text field focus).

2. **View & ViewModel Binding (Pending)**
   - Create base classes/interfaces for `View` and `ViewModel`.
   - Implement a C++ mechanism to mimic data binding (e.g., using `std::function` callbacks, observer patterns, or a signal/slot mechanism).
   - Example implementation: A `Button` View that listens to an `ActionModel` in the ViewModel.

3. **Model Layer (Pending)**
   - Define guidelines for pure state and business logic models that the ViewModels will wrap and expose to the Views.

4. **Event Routing (Pending)**
   - Ensure events flow correctly: Controller -> ViewModel -> Model, and state changes flow: Model -> ViewModel -> View.
