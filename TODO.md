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
    - Make a way to load transparent materials

## Physics

## Particles
    - Keep adding stuff.. rn its very simple
    - https://alextardif.com/Particles.html
    - Currently particles are z-facing.. make them billboards or something

## Graphics
    - Shader permutations https://therealmjp.github.io/posts/shader-permutations-part1/
    - we could support immediate trivertices and convert to default ubervertex on demand, for simpler API
    - in ogl.h we have is_image for binding to compute shaders image2d's its DUMB just have shader introspection here..

## Assets
    - With current handling textures can't be customized (e.g different magFilter)
    - Discerning asset type based on asset tag suffix is VERY bad and sad - Maybe asset should be discriminated union, so we don't have code duplication..
    - Do we need RenderBundle to be an asset?
    - Asset streaming (I think assets should be obtainable from assetid for this)

## General
    - When loading fucking 3d models compute the bounded box and make the world matrix transform so that center of aabb is (0,0)
    - We could.. specify this though, some models shouldn't be messed with the offset is important many times (center = true?)
    - Make a way to only statically link everything?
    - C99?
## ogl
    - We need to make the introspection system better, we NEED a way to default initialize textures AND texture arrays like t[0] t[1]
## demo
