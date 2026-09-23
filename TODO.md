## Gui
    - Better animations
    - Regular image support
    - Unicode support
    - More widgets?
    - Keyboard Navigation..

## Entity
    - Signals
    - Implement the reuse logic for staled entity indices
    - Entity tree, maybe use a stack? Not sure if needed for now

## Math

## GLTF
    - .glb (next?)

## Physics
    - Integrate Box3D (dont)

## Particles
    - Keep adding stuff.. rn its very simple
    - https://alextardif.com/Particles.html

## Graphics
    - Shadows!!!!
    - Shader permutations https://therealmjp.github.io/posts/shader-permutations-part1/

## Assets
    - With current handling textures can't be customized (e.g different magFilter)
    - Discerning asset type based on asset tag suffix is VERY bad and sad
    - Maybe asset should be discriminated union, so we don't have code duplication..
    - Do we need RenderBundle to be an asset?

## demo
    - Skeletal Animated entity (Hero) that can navigate in space
    - Enviroment has some triggers like buttons that do stuff on the world
    - Animated camera for events/cutscenes
    - horde enemies + killing them (like vampire survivors)
    - First level could be a big open space where you press a button to open a door with enemies, killing them resets level
