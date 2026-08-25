#ifndef JSON_UTIL_H__
#define JSON_UTIL_H__

#include "base/helper.h"
// TODO: error system needs more work

typedef enum {
  JSON_TOKEN_KIND_COMMA,
  JSON_TOKEN_KIND_COLON,
  JSON_TOKEN_KIND_LPAREN,
  JSON_TOKEN_KIND_RPAREN,
  JSON_TOKEN_KIND_LBRACKET,
  JSON_TOKEN_KIND_RBRACKET,
  JSON_TOKEN_KIND_LCURLY,
  JSON_TOKEN_KIND_RCURLY,
  JSON_TOKEN_KIND_NUMBER,
  JSON_TOKEN_KIND_TRUE,
  JSON_TOKEN_KIND_FALSE,
  JSON_TOKEN_KIND_NULL,
  JSON_TOKEN_KIND_STRING,
} Json_Token_Kind;
typedef struct {
  str8 buf;
  Json_Token_Kind kind;
} Json_Token;
typedef struct {
  Json_Token *tokens;
  u64 count;
} Json_Tokens;


//@FIXME: This is not correct, FIX this
static int json_tok_count_number(char* num_str) {
    int count = 0;
    for(u32 i = 0; i < cstr_count(num_str); ++i, ++count) {
        char c = num_str[i];
        if (c != 'e' && c != 'E' && c != '.' && c != '-' && (c < '0' || c > '9'))break;
    }
    return count;
}

static int json_tok_count_string(char* s) {
    int count = 0;
    int quote_count = 0;
    for(u32 i = 0; i < cstr_count(s); ++i, ++count) {
        if (quote_count == 2)break;
        char c = s[i];
        if (c == '\"')quote_count+=1;
    }
    return count;
}

static Json_Token json_tok_get(Json_Tokens tokens, u32 idx) {
  return (idx >= tokens.count) ? (Json_Token){} : tokens.tokens[idx];
}

static Json_Tokens json_tokenize(Arena *arena, str8 json_str) {
    Json_Tokens tokens = {};

    // We pre-parse the Json one time to get the token count :/
    // maybe TODO: optimize this
    if (arena) {
      tokens = json_tokenize(nullptr, json_str);
      tokens.tokens = arena_push_array(arena, Json_Token, tokens.count);
      tokens.count = 0;
    }

    int count = 0;
    for (u32 i = 0; i < json_str.count; i+=count) {
        Json_Token token = {};
        char *c = (char*)&json_str.data[i];
        count = 1;
        switch (c[0]) {
           case ',':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_COMMA};
                break;
            case ':':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_COLON};
                break;
            case '(':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_LPAREN};
                break;
            case ')':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_RPAREN};
                break;
            case '[':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_LBRACKET};
                break;
            case ']':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_RBRACKET};
                break;
            case '{':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_LCURLY};
                break;
            case '}':
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_RCURLY};
                break;
            case '-':
            case '+':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                count = json_tok_count_number(c);
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_NUMBER};
                break;
            case 't':
                count = cstr_count("true");
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_TRUE};
                break;
            case 'f':
                count = cstr_count("false");
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_FALSE};
                break;
            case 'n':
                count = cstr_count("null");
                token = (Json_Token) {STR8(c, count), JSON_TOKEN_KIND_NULL};
                break;
            case '\"':
                count = json_tok_count_string(c);
                assert(count);
                // +1 -2 to remove the quotes '\"' FIXME: this is hacky
                token = (Json_Token) {STR8(c+1, count-2), JSON_TOKEN_KIND_STRING};
                break;
            case ' ':
            case '\\':
            case '\t':
            case '\n':
            default:
                break;
        }
        if (arena && token.buf.data) {
          tokens.tokens[tokens.count] = token; 
        }
        if (token.buf.data) {
          tokens.count += 1;
        }
    }

    return tokens;
}


static Json_Tokens json_tokenize_cstr(Arena *arena, char *json_str) {
  return json_tokenize(arena, STR8(json_str, cstr_count(json_str)));
}

static void json_tok_print(Json_Tokens tokens) {
  for (u64 idx = 0; idx < tokens.count; idx+=1) {
    Json_Token tok = json_tok_get(tokens, idx);
    printf ("%.*s\n", STR8_VARG(tok.buf));
  }
}

typedef struct Json_Element Json_Element;
struct Json_Element {
  str8 label;
  str8 value;

  Json_Element *first;
  Json_Element *next;
};

typedef enum {
  JSON_PARSER_ERROR_NONE = 0,
  JSON_PARSER_ERROR_UNEXPECTED_TOKEN,
  // ..
}Json_Parser_Error;

typedef struct {
  Json_Tokens tokens;
  u64 pos;
  Arena *arena;

  Json_Parser_Error error_state;
  u32 error_token;
}Json_Parser;

static Json_Token json_parser_get_tok(Json_Parser *parser) {
  return json_tok_get(parser->tokens, parser->pos);
}

static void json_parser_eat_tok(Json_Parser *parser, Json_Token_Kind expected_tok_kind) {
  Json_Token tok = json_parser_get_tok(parser);

  if (tok.kind == expected_tok_kind) {
    parser->pos += 1;
  } else {
    parser->error_state = JSON_PARSER_ERROR_UNEXPECTED_TOKEN;
    parser->error_token = parser->pos;
  }
}

static s32 json_count_children(Json_Element *root) {
  Json_Element *iter = root->first;
  s32 children_count = 0;
  while (iter != nullptr) {
    children_count+=1;
    iter = iter->next;
  }

  return children_count;
}

static s32 json_count_siblings(Json_Element *root) {
  Json_Element *iter = root;
  s32 sib_count = 0;
  while (iter != nullptr) {
    iter = iter->next;
    if (iter) sib_count+=1;
  }

  return sib_count;
}

static Json_Element* json_lookup(Json_Element *root, str8 label) {
  Json_Element *iter = root->first;
  while (iter != nullptr && !str8_eq(iter->label, label))iter = iter->next;

  return iter;
}

static Json_Element* json_parse_element(Json_Parser *parser, str8 label);
static str8 json_parse_primitive(Json_Parser *parser) {
  str8 prim = {};
  Json_Token tok = json_parser_get_tok(parser);
  bool found = true;
  if (tok.kind == JSON_TOKEN_KIND_TRUE) {
    prim = tok.buf;
  } else if (tok.kind == JSON_TOKEN_KIND_FALSE){
    prim = tok.buf;
  } else if (tok.kind == JSON_TOKEN_KIND_STRING) {
    prim = tok.buf;
  } else if (tok.kind == JSON_TOKEN_KIND_NULL){
    prim = tok.buf;
  } else if (tok.kind == JSON_TOKEN_KIND_NUMBER){ 
    prim = tok.buf;
  } else {
    found = false;
  }

  if (found)parser->pos+=1;

  return prim;
}

static Json_Element* json_parse_list(Json_Parser *parser, str8 label, Json_Token_Kind end_token, bool has_labels);
static Json_Element* json_parse_element(Json_Parser *parser, str8 label) {
  Json_Token tok = json_parser_get_tok(parser);

  // Try to parse a sublist (if its an object or array)
  Json_Element *sublist = nullptr;
  if (tok.kind == JSON_TOKEN_KIND_LCURLY) {
    json_parser_eat_tok(parser, JSON_TOKEN_KIND_LCURLY);
    sublist = json_parse_list(parser, label, JSON_TOKEN_KIND_RCURLY, true);
  }
  if (tok.kind == JSON_TOKEN_KIND_LBRACKET) {
    json_parser_eat_tok(parser, JSON_TOKEN_KIND_LBRACKET);
    sublist = json_parse_list(parser, label, JSON_TOKEN_KIND_RBRACKET, false);
  }

  // Try to parse actual value (if its a primitive) and return the elements
  Json_Element *e = arena_push_array(parser->arena, Json_Element, 1);
  e->first = sublist;
  e->label = label;
  e->next = nullptr;
  e->value = json_parse_primitive(parser);

  return e;
}

static Json_Element* json_parse_list(Json_Parser *parser, str8 label, Json_Token_Kind end_token, bool has_labels) {
  Json_Element *first = nullptr;
  Json_Element *last = nullptr;

  str8 labelv = {};
  while (json_parser_get_tok(parser).kind != end_token) {
    if (has_labels) {
      // parse the label
      labelv = json_parse_primitive(parser);
      json_parser_eat_tok(parser, JSON_TOKEN_KIND_COLON);
    }
    // parse the value
    Json_Element *value = json_parse_element(parser, label);
    value->label = labelv;
    assert(value);
    sll_queue_push(first, last, value);

    if (json_parser_get_tok(parser).kind == JSON_TOKEN_KIND_COMMA) json_parser_eat_tok(parser, JSON_TOKEN_KIND_COMMA);
  }
  json_parser_eat_tok(parser, end_token);

  return first;
}

static Json_Element *json_parse_tokens(Arena *arena, Json_Tokens tokens) {
  Json_Parser p = (Json_Parser) {
    .tokens = tokens,
    .pos = 0,
    .arena = arena,
    .error_state = JSON_PARSER_ERROR_NONE,
  };
  Json_Element *e = json_parse_element(&p, (str8){});

  if (p.error_state != JSON_PARSER_ERROR_NONE) {
    Json_Token tok = json_tok_get(p.tokens, p.error_token);
    printf("parsing Json file failed with error [%d] at token [%d]=[%.*s]\n", p.error_state, p.error_token, STR8_VARG(tok.buf));
    return nullptr;
  } 
  return e;
}

static Json_Element *json_parse(Arena *arena, str8 json_str) {
  Json_Tokens tokens = json_tokenize(arena, json_str);
  Json_Element *root = json_parse_tokens(arena, tokens);

  return root;
}

static char *test_json_str = "{ \
  \"scene\": 0, \
  \"scenes\" : [ \
    { \
      \"nodes\" : [ 0 ] \
    } \
  ], \
 \
  \"nodes\" : [ \
    { \
      \"mesh\" : 0 \
    } \
  ], \
 \
  \"meshes\" : [ \
    { \
      \"primitives\" : [ { \
        \"attributes\" : { \
          \"POSITION\" : 1 \
        }, \
        \"indices\" : 0, \
        \"material\" : 0, \
        \"mode\" : 4, \
      } ] \
    } \
  ], \
 \
  \"buffers\" : [ \
    { \
      \"uri\" : \"data:application/octet-stream;base64,AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAA=\", \
      \"byteLength\" : 44 \
    } \
  ], \
  \"bufferViews\" : [ \
    { \
      \"buffer\" : 0, \
      \"byteOffset\" : 0, \
      \"byteLength\" : 6, \
      \"target\" : 34963 \
    }, \
    { \
      \"buffer\" : 0, \
      \"byteOffset\" : 8, \
      \"byteLength\" : 36, \
      \"target\" : 34962 \
    } \
  ], \
  \"accessors\" : [ \
    { \
      \"bufferView\" : 0, \
      \"byteOffset\" : 0, \
      \"componentType\" : 5123, \
      \"count\" : 3, \
      \"type\" : \"SCALAR\", \
      \"max\" : [ 2 ], \
      \"min\" : [ 0 ] \
    }, \
    { \
      \"bufferView\" : 1, \
      \"byteOffset\" : 0, \
      \"componentType\" : 5126, \
      \"count\" : 3, \
      \"type\" : \"VEC3\", \
      \"max\" : [ 1.0, 1.0, 0.0 ], \
      \"min\" : [ 0.0, 0.0, 0.0 ] \
    } \
  ], \
  \"materials\": [\
    {\
      \"pbrMetallicRoughness\": {\
        \"baseColorFactor\": [\
          0.2,\
          0.3,\
          0.9,\
          1.0\
        ],\
        \"metallicFactor\": 0.0\
      },\
      \"emissiveFactor\": [ 0.0, 0.0, 0.0 ],\
      \"name\": \"Material_001-effect\"\
    }\
  ],\
  \"asset\" : { \
    \"version\" : \"2.0\" \
  } \
} ";

//Ref: https://github.khronos.org/glTF-Tutorials/gltfTutorial/

#endif
