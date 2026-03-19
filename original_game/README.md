# Urban Chaos — Original Game Source

Original source code of Urban Chaos (1999, MuckyFoot Productions), updated by community contributors
to build and run on modern Windows.

For build instructions see [SETUP.md](SETUP.md).

## Credits

| | |
|-|-|
| **Mike Diskett** ([dizzy2003/MuckyFoot-UrbanChaos](https://github.com/dizzy2003/MuckyFoot-UrbanChaos)) | Original source code, released under MIT license (2017) |
| **Kai Stüdemann** ([kstuedem/MuckyFoot-UrbanChaos](https://github.com/kstuedem/MuckyFoot-UrbanChaos)) | VS2017 support, SDL2/OpenAL audio, removed legacy DirectSound/DirectPlay, rendering fixes, inline assembly removal |
| **Jordan Davidson** ([jordandavidson/MuckyFoot-UrbanChaos](https://github.com/jordandavidson/MuckyFoot-UrbanChaos)) | vcpkg integration, VS2019 support |
| **ultimabear** | Minor additional fixes on top of the above |

The code in this folder is based on a further updated version of Jordan Davidson's fork. Kept as a **read-only reference** for the
[OpenChaos](../README.md) modernization project.

---

## Original README by Mike Diskett

## MuckyFoot-UrbanChaos

This is a snapshot of (the 1999 PC/PSX/DreamCast game) Urban Chaos source code recently retreived from an old SourceSafe backup pulled from an old MuckyFoot server.

The games source code is in the fallen directory (the working title for Urban Chaos)

At the time it was compiled with Visual Studio 6 I believe and possibly DirectX 6 and is entirey written in c (despite using cpp files).
(I notice also some Glide code in there) I have succesfully loaded the projects with VS2013 though.

There is also source code for the psx version, and maybe even the Dream Cast port.

Urban Chaos had its own map editor and animation keyframe editor, these ran if I remember correctly under a window system
implemented by Guy Simmons, the code is included in the Editor folder

MFlib1 and MFStdLib are some generic libraries shared across Urban Chaos and possibly Startopia
MuckyBasic is a inhouse scripting language written by Mark Adami
Thrust is a side project by Mark Adami

Note no data is included, and the source data used by the editor/Engine is likely somewhat different to the data used by the final disk image
(all though much of it is probably identical)

I am making this code public mainly out of historical interest, I doubt theres anything of any actual use to anyone, allthough people do
occasionally still ask me about data formats so they can dig them out themsleves now...


*BrainDump*

I tried compiling the code, but theres a lot of errors..

We used DirectX before it did hardware vertex transforms, at some point VertexBuffers came along and I seem to recall Eddie Edwards implementing their use.

a prim is simply a mesh (lampost etc)

There is no soft skinning characters are seperate meshes

The floor is a heightmap with a vertical offset possible for the kerbs see pap.h

The building meshes were created on the fly from wall data that could generate N stories high of side wall building.cpp

Fence meshes and fire escape meshes were similarily created on the fly

The editor allowed placement of waypoints and control code for ai. as well as map creation

The data set was limited to run on a 1 Meg Playstation1 (PSX)

Theres code for features that never shipped like a motorbike and a grappling hook with rope physics, also once
mav is the sequel to nav, it handles mavigation :)

I also had a tendency to increment the letter of a file type with new versions so map.h eventually became pap.h (much of the code was written with a 8.3 filename restriction)

yes we had pee physics

Thing could be considered a cpp base class

MapWho is a bullfrog thing, essentially a 2d spatial array that objects in the world attach themselves to as a linked list for fast spatial lookups

once upon a time the building all had procedural internals, also sewer systems..




Mike Diskett
