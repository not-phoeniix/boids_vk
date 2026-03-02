#!/bin/bash

# copy so file
cp ~/Projects/cpp/render_thing/bin/librender_thing.so ~/Projects/cpp/boids_vk/lib/librender_thing.so
cp ~/Projects/cpp/render_thing/bin/librender_thing.so ~/.local/lib/librender_thing.so

# copy headers
rm -r ~/Projects/cpp/boids_vk/include/render_thing
cp -r ~/Projects/cpp/render_thing/include ~/Projects/cpp/boids_vk/include/render_thing
