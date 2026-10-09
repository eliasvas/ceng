## Gui
    - Better animations
    - Regular image support
    - Unicode support
    - More widgets?
    - Keyboard Navigation..

## Entity
    - Signals (Should be used for score increase as coins are killed ok)
    - Make camera an entity
    - Make movement (WASD) use camera for positioning
    - EntityRenderCommand should have entity pointer and skip some stuff no?
    - Our models can be loaded offseted from (0,0) how do we reconcile in entity system?

## Math

## GLTF
    - Sparse accessors
    - Can't animated multiple characters becaus elocal transforms inside ModelInfo

## Physics

## Particles
    - Keep adding stuff.. rn its very simple
    - https://alextardif.com/Particles.html
    - Currently particles are z-facing.. make them billboards or something

## Graphics
    - Shadows!!!!
    - Shader permutations https://therealmjp.github.io/posts/shader-permutations-part1/

## Assets
    - With current handling textures can't be customized (e.g different magFilter)
    - Discerning asset type based on asset tag suffix is VERY bad and sad - Maybe asset should be discriminated union, so we don't have code duplication..
    - Do we need RenderBundle to be an asset?
    - Asset streaming (I think assets should be obtainable from assetid for this)

## General
    - When loading fucking 3d models compute the bounded box and make the world matrix transform so that center of aabb is (0,0)
    - We could.. specify this though, some models shouldn't be messed with the offset is important many times (center = true?)

## demo
