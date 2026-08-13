//#define INPUT_IMPLEMENTATION
#include "core/input.h"
#include "base/base_inc.h"

#include "game.h"
#include "gui/gui.h"
#include "entity.h"

#define ASSET_MGR_IMPLEMENTATION
#include "asset/asset_mgr.h"



static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
unsigned char * base64_decode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

extern void platform_play_sound(const char *sound);

// Just for testing
Tri_Vertex gltf_verts[3] = {};

void game_init(Game_State *gs) {
  entity_store_init();

  // Make the hero
  Entity *hero = entity_store_add();
  hero->kind = ENTITY_KIND_HERO;
  hero->dynamic = true;
  hero->box = (Phys_Box) {
    .pos = HMM_V3(1,4,1),
    .col_off = HMM_V3(0,0,0),
    .hdim = HMM_V3(0.3, 0.5, 0.3),
    .col_hdim = HMM_V3(0.5,0.5,0.5),
  };
  hero->col = v4m(0.9,0.4,0.3,1.0);
  
  // Make a test
  for (s32 width = -3; width <= 3; width+=6) {
    for (s32 height = 0; height < 3; height +=1) {
      Entity *test = entity_store_add();
      test->dynamic = false;
      test->box = (Phys_Box) {
        .pos = HMM_V3(width,height,1),
        .col_off = HMM_V3(0,0,0),
        .col_hdim = HMM_V3(0.5,0.5,0.5),
        .hdim = HMM_V3(0.5,0.5,0.5),
      };
      test->col = v4m(0.2,0.4,0.9,1.0);
    }
  }

  Entity *ground = entity_store_add();
  ground->dynamic = false;
  ground->box = (Phys_Box) {
    .pos = HMM_V3(0,-1.01,0),
    .col_off = HMM_V3(0,0,0),
    .col_hdim = HMM_V3(4,1,4),
    .hdim = HMM_V3(4,1,4),
  };
  ground->col = v4m(0.4,0.4,0.4,1.0);

  gs->view = HMM_LookAt_RH(HMM_V3(0,8,10), HMM_V3(0,0,0), HMM_V3(0,1,0));


  gui_init(gs->frame_arena, &gs->font, &gs->input);

  /////////////////////////////////////////////////////////////
  /// Make this into the assert test for the json parser too1
  /////////////////////////////////////////////////////////////
  gltf_verts[0] = (FRZ_Vertex) {.pos = v3m(-0.0, -0.5, +0.5), .uv = v2m(0,0), .color = FRZ_RED,};
  gltf_verts[1] = (FRZ_Vertex) {.pos = v3m(+0.0, -0.5, +0.5), .uv = v2m(1,0), .color = FRZ_GREEN,};
  gltf_verts[2] = (FRZ_Vertex) {.pos = v3m(+0.0, +0.5, +0.5), .uv = v2m(1,1), .color = FRZ_WHITE,};
  // parse gltf and get the actual data
  Json_Element *root = json_parse(gs->frame_arena, test_json_str);
  Json_Element* scene = json_lookup(root, STR8L("scene"));
  assert(str8_eq(scene->label, STR8L("scene")) && str8_to_int(scene->value) == 0);

  // 0. Parse meshes
  typedef struct Mesh_Data {s32 pos_idx; s32 indices_idx;} Mesh_Data;
  Json_Element* meshes_json = json_lookup(root, STR8L("meshes_json")); assert(meshes_json);
  Mesh_Data *meshes = arena_push_array(gs->frame_arena, Mesh_Data, json_count_children(meshes_json));
  s32 mesh_idx = 0;
  for (Json_Element *mesh = meshes_json->first; mesh != nullptr; mesh = mesh->next, mesh_idx+=1) {
    Json_Element* primitives_json = json_lookup(meshes_json, STR8L("primitives")); assert(primitives_json);
    for (Json_Element *prim = primitives_json->first; prim != nullptr; prim = prim->next) {
      Json_Element* attribs_json = json_lookup(prim, STR8L("attributes")); assert(attribs_json);

      // WHY attribs_json->first->first
      for (Json_Element *attrib = attribs_json->first->first ; attrib != nullptr; attrib = attrib->next) {
        //printf("IAM ATTRIB %.*s -> %.*s \n", STR8_VARG(attrib->label), STR8_VARG(attrib->value));
        if (str8_eq(attrib->label, STR8L("POSITION"))) {
          meshes[mesh_idx].pos_idx = str8_to_int(attrib->value);
        }
      }
      Json_Element* indices = json_lookup(attribs_json, STR8L("indices")); assert(indices);
      meshes[mesh_idx].indices_idx = str8_to_int(indices->value);
    }
  }

  // 1. Parse buffers
  Json_Element* buffers_json = json_lookup(root, STR8L("buffers")); assert(buffers_json);
  str8 *buffers = arena_push_array(gs->frame_arena, str8, json_count_children(buffers_json));
  s32 buf_idx = 0;
  for (Json_Element *b= buffers_json->first; b != nullptr; b = b->next, buf_idx+=1) {
    Json_Element* data = json_lookup(b, STR8L("uri")); assert(data);
    Json_Element* byte_len = json_lookup(b, STR8L("byteLength")); assert(byte_len);

    buffers[buf_idx] = str8_substr(data->value, str8_find_needle(data->value, STR8L(","))+1, data->value.count); 
    //buffers[buf_idx].count = str8_to_int(byte_len->value);

    size_t len = 0;
    unsigned char * raw = base64_decode(buffers[buf_idx].data,buffers[buf_idx].count, &len);
    buffers[buf_idx].data = raw;
    buffers[buf_idx].count = len;

    assert(len == (size_t)str8_to_int(byte_len->value));

    // aren't buffers always the length of the 'uri' string?
    printf("len %.*s buf: %ld\n", STR8_VARG(byte_len->value), buffers[buf_idx].count);
    //assert(str8_to_int(byte_len->value) == buffers[buf_idx].count);
  }

  // 2. Parse bufferViews
  Json_Element* buffer_views_json = json_lookup(root, STR8L("bufferViews")); assert(buffer_views_json);
  str8 *buffer_views = arena_push_array(gs->frame_arena, str8, json_count_children(buffer_views_json));
  s32 view_idx = 0;
  for (Json_Element *b= buffer_views_json->first; b != nullptr; b = b->next, view_idx+=1) {
    Json_Element* buf_idx = json_lookup(b, STR8L("buffer")); assert(buf_idx);
    Json_Element* offset  = json_lookup(b, STR8L("byteOffset")); assert(offset);
    Json_Element* length  = json_lookup(b, STR8L("byteLength")); assert(length);
    //Json_Element* target = json_lookup(buffer_views_json, STR8L("target")); assert(target);

    buffer_views[view_idx].data = str8_to_int(offset->value) + buffers[str8_to_int(buf_idx->value)].data;
    buffer_views[view_idx].count = str8_to_int(length->value);
    printf("--- %.*s\n", STR8_VARG(buffer_views[view_idx]));
  }

  // 3. Parse accessors
  Json_Element* accessors_json = json_lookup(root, STR8L("accessors")); assert(accessors_json);
  str8 *accessors = arena_push_array(gs->frame_arena, str8, json_count_children(accessors_json));
  s32 acc_idx = 0;
  for (Json_Element *a= accessors_json->first; a != nullptr; a = a->next, acc_idx+=1) {
    Json_Element* bufv_idx = json_lookup(a, STR8L("bufferView")); assert(bufv_idx);
    Json_Element* offset  = json_lookup(a, STR8L("byteOffset")); assert(offset);
    //Json_Element* compType = json_lookup(a, STR8L("componentTYpe")); assert(compType);
    Json_Element* count = json_lookup(a, STR8L("count")); assert(count);
    Json_Element* type = json_lookup(a, STR8L("type")); assert(type);
    //Json_Element* max = json_lookup(a, STR8L("max")); assert(max);
    //Json_Element* min = json_lookup(a, STR8L("min")); assert(min);

    accessors[acc_idx].data = str8_to_int(offset->value) + buffer_views[str8_to_int(bufv_idx->value)].data;
    accessors[acc_idx].count = str8_to_int(count->value);
  }

  Mesh_Data *md = &meshes[0];
  gltf_verts[0].pos.x = ((float*)accessors[md->pos_idx].data)[0];
  gltf_verts[0].pos.y = ((float*)accessors[md->pos_idx].data)[1];
  gltf_verts[0].pos.z = ((float*)accessors[md->pos_idx].data)[2];

  gltf_verts[1].pos.x = ((float*)accessors[md->pos_idx].data)[3];
  gltf_verts[1].pos.y = ((float*)accessors[md->pos_idx].data)[4];
  gltf_verts[1].pos.z = ((float*)accessors[md->pos_idx].data)[5];

  gltf_verts[2].pos.x = ((float*)accessors[md->pos_idx].data)[6];
  gltf_verts[2].pos.y = ((float*)accessors[md->pos_idx].data)[7];
  gltf_verts[2].pos.z = ((float*)accessors[md->pos_idx].data)[8];

}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->wdim.x, gs->wdim.y);
  gs->proj = HMM_Perspective_RH_NO(45, gs->game_viewport.w/gs->game_viewport.h, 0.1, 100);


  // Camera stuff (This is wrong because we post-multiply.. FIXME)
  v2 mouse_delta = input_get_mouse_delta(&gs->input);
  if (input_mkey_down(&gs->input, INPUT_MOUSE_RMB)) { 
    HMM_Quat rot_q = HMM_QFromAxisAngle_RH(HMM_V3(0,1,0), mouse_delta.x*dt);
    gs->view = HMM_Mul(gs->view, HMM_QToM4(rot_q));
  }
  if (input_mkey_down(&gs->input, INPUT_MOUSE_RMB)) { 
    HMM_Quat rot_q = HMM_QFromAxisAngle_RH(HMM_V3(1,0,0), mouse_delta.y*dt);
    gs->view = HMM_Mul(gs->view, HMM_QToM4(rot_q));
  }
}

void game_draw_origin_grid(Game_State *gs, s32 cell_count) {
  s32 line_count_per_axis = cell_count + 1; 
  Tri_Vertex *points = arena_push_array(gs->frame_arena, Tri_Vertex, line_count_per_axis*4);
  color c1 = v4m(0.7,0.7,0.7,1);

  HMM_Mat4 model = HMM_Translate(HMM_V3(0, 0, 0));
  HMM_Mat4 mvp = HMM_Mul(HMM_Mul(gs->proj, gs->view), model);

  s32 point_idx = 0;

#if 1
  for (s32 line_x = 0; line_x < line_count_per_axis; line_x +=1) {
    v3 start = v3m(-cell_count/2.0,0, line_x - cell_count/2.0);
    v3 end   = v3m(+cell_count/2.0,0, line_x - cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }
#endif

  for (s32 line_z = 0; line_z < line_count_per_axis; line_z +=1) {
    v3 start = v3m(line_z - cell_count/2.0, 0,-cell_count/2.0);
    v3 end   = v3m(line_z - cell_count/2.0, 0,+cell_count/2.0);

    points[point_idx++] = (Tri_Vertex) {.pos = start, .color = c1};
    points[point_idx++] = (Tri_Vertex) {.pos = end, .color = c1};
  }

  assert(point_idx == line_count_per_axis*4);
  rn_imm_verts(gs->game_viewport, points, line_count_per_axis * 4, OGL_PRIM_TYPE_LINE, (m4*)&mvp);

  rn_imm_verts(gs->game_viewport, gltf_verts, 3, OGL_PRIM_TYPE_TRIANGLE, (m4*)&mvp);
}

void game_render(Game_State *gs, float dt) {
  //m4 model = m4_mult(m4_translate(v3m(0,0,0)), m4_rotate(gs->time_sec*3.14, v3m(0,1,0)));

  // 0. Draw grid
  game_draw_origin_grid(gs, 10);
  entity_store_update_render(gs, dt);

  // Simple test for quad rendernig
#if 0
  // TODO: Maybe we should push/pop asset ids for textures??
  // Sample quad for renderer architecture
  R_Quad q = (R_Quad){
    .dst_rect = rec(300,200,500,500),
    .src_rect = rec(0,0,128,80),
    .c = col(1.0,1.0,1.0,1.0),
    .rot_deg = 9.0 * gs->time_sec,
    .corner_radius = 20.0,
    .softness = 4.0,
    .tex = (Ogl_Tex*)am_get(asset_id_from_path(STR8L("white.png"))),
  };
  r2d_push_quad(r2d_pass_front(), q);

  // For test
  //r2d_flush_all();

  q = (R_Quad){
    .dst_rect = rec(500,200,500,500),
    .src_rect = rec(0,0,128,80),
    .c = col(1.0,1.0,1.0,1.0),
    .rot_deg = 19.0 * gs->time_sec,
    .corner_radius = 20.0,
    .softness = 4.0,
    .tex = (Ogl_Tex*)am_get(asset_id_from_path(STR8L("atlas.png"))),
  };
  r2d_push_quad(r2d_pass_front(), q);

  //r2d_flush_all();

#endif


  // Gui Test
#if 1
  // Perform a reload if reset button is clicked
  gui_begin(gs->game_viewport, dt);
  gui_push_text_alignment(GUI_TEXT_ALIGNMENT_LEFT);

  static Gui_Scroll_Data sdata = {
    .item_px = 60, // FIXME change this to 60 to see some weird stuff..
    .item_count = 8,
    .scroll_bar_px = 15,
    .scroll_button_px = 15,
    .scroll_button_color = col(0.5,1,0.4,1),
    .scroll_speed = 1,
    .scroll_percent = 0,
  };

  Gui_Signal scroll_list = gui_scroll_list_begin(STR8L("MyScrollTest"), GUI_AXIS_Y, &sdata);
  assert(scroll_list.box);
  //gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PIXELS, 100.0, 0.0});
  gui_push_pref_width((Gui_Size){.kind = GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 0.0});
    if (gui_button(STR8L("AAAA")).sflags & GUI_SIGNAL_FLAG_LMB_PRESSED) printf("AAAA\n");
    gui_button(STR8L("BBBB"));
    gui_button(STR8L("CCCC"));
    gui_button(STR8L("DDDD"));
    gui_button(STR8L("EEEE"));
    gui_button(STR8L("FFFF"));
    gui_button(STR8L("GGGG"));
    gui_button(STR8L("HHHH"));
    gui_pop_pref_width();
  gui_scroll_list_end(STR8L("MyScrollTest"));
  gui_end();
#endif

}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

