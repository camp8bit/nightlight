Nightlight
==========

Nightlight is a protocol and base Arduino library for making a collection of peer-to-peer audiovisual toys.

See examples/controllable_node for a simple example.

This is very much a work in progress and you should expect some heavy refactoring.

Concepts
--------

Multi-device networking can get kind of complicated, and so Nightlight uses a state machine on each device to keep track of things. Each state is a sublcass of `NightlightState`, and has methods for responding to serial messages, radio messages, and handling timers.

The transitions between states aren't usually hard-coded: instead, pointers to other states are passed in as parameters, in a simple form of depdency injection.

The Nightlight library will come with a number of states for common use-cases (right not "an open node, waiting for a controller", and "a node under control" are the two provided), or developers can make their own from scratch.

The actual controlling of output pins is not hard coded into the state machines. Output objects with methods such as `on()` and `off()` are referenced, by passing pointers into setters on the state objects (again some simple dependency injection). This means that the logic for radio communications is seperate from the logic for making things happen on your device.

Key Classes
-----------

 * `Nightlight`: Represents your application. Create one of these, load in states and a radio.
 * `NightlightState`: Represents one state of your Nightlight app.
 * `DigitalOutput`: A simple output device, that can turn a single digital pin on or off.