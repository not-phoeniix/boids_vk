# boids_vk
**boids_vk** is a 3D boids simulation rendered in Vulkan! The aim of this project is to practice data-oriented design practices to create a complex and efficient simulation of a ***ton of boids*** (10,000 boids to be exact).

## Features/Performance
Controls:
- **WASD:** fly around the scene relative to camera orientation
- **E/Q:** fly up/down, absolute Y coordinate changes
- **LMB (hold & drag):** Look/Rotate the camera (in first person) around the scene
- **R:** randomize lights
- **ESC:** quit :]

This project features instanced rendering, multithreaded simulation, dynamic lighting, spatial partitioning, and other goodies to make it look really pretty and run really fast!

My thin & light laptop equipped with a Ryzen 7 7840U (and just integrated graphics) runs this project ~40fps on average. Simulation times are printed to the console and get on average around 20-25ms for 10,000 boids simulated. Simulation times depend on your CPU, the more cores you have the better (the thread pool likes that <3)

## Screenshots
This project is really pretty! Check out these screenshots :D

<span align="center">
    <img src="screenshots/sc_01.png" alt="A sphere of boids from a distance" width="32%">
    <img src="screenshots/sc_02.png" alt="Inside a sphere of boids swarming" width="32%">
    <img src="screenshots/sc_03.png" alt="Up-close view of multi-colored bird-shaped boid model" width="32%">
</span>

## Dependencies
All library dependencies are included in the git repo already. The project has been created to support building on Linux using GNU Make. 

The libraries used in this project are as follows:
- vk-bootstrap: https://github.com/charles-lunarg/vk-bootstrap
- stb_image: https://github.com/nothings/stb
- tinyobjloader: https://github.com/tinyobjloader/tinyobjloader

## Building
Only works on Linux for now. Run `make` in the root of the project folder, build is outputted at `bin/build`. Also make sure to toss `lib/librender_thing.so` in your dynamically linked libraries path. 

You can also run `make run` to automatically build & run the project and set up the path for the `.so` file <3

## License
boids_vk is licensed under the **MIT License**. Please see the [LICENSE](LICENSE) document for more details.
