// Tippse - Encoding UTF-8 - Encode/Decode of UTF-8 codepoints

#include "utf8.h"

struct encoding* encoding_utf8_create(void) {
  struct encoding_utf8* self = (struct encoding_utf8*)malloc(sizeof(struct encoding_utf8));
  self->vtbl.create = encoding_utf8_create;
  self->vtbl.destroy = encoding_utf8_destroy;
  self->vtbl.name = encoding_utf8_name;
  self->vtbl.character_length = encoding_utf8_character_length;
  self->vtbl.encode = encoding_utf8_encode;
  self->vtbl.decode = encoding_utf8_decode;
  self->vtbl.visual = encoding_utf8_visual;
  self->vtbl.next = encoding_utf8_next;

  return (struct encoding*)self;
}

void encoding_utf8_destroy(struct encoding* base) {
  struct encoding_utf8* self = (struct encoding_utf8*)base;
  free(self);
}

const char* encoding_utf8_name(void) {
  return "UTF-8";
}

size_t encoding_utf8_character_length(struct encoding* base) {
  return 4;
}

codepoint_t encoding_utf8_visual(struct encoding* base, codepoint_t cp) {
  if (cp>UNICODE_CODEPOINT_MAX) {
    return UNICODE_CODEPOINT_BAD;
  } else if (cp<0x20) {
    return cp+0x2400;
  }

  return cp;
}

codepoint_t encoding_utf8_decode_boundary(struct encoding* base, struct stream* stream, size_t* used) {

  codepoint_t cp;
  size_t use = 0;

  uint8_t c = stream_read_forward(stream);
  use++;
  if (c<0x80) { // 0, 256, 128
    cp = c;
    goto okay;
  } else {
    if (c>=0xc2) { // 128, 256, 192
      if (c<0xe1) { // 194, 256, 225
        if (c>=0xe0) { // 194, 225, 209
          uint8_t c = stream_read_forward(stream);
          use++;
          if (c>=0xa0) { // 0, 256, 128
            if (c<0xc0) { // 160, 256, 208
              cp = (uint8_t)(c&0x7f);
              uint8_t c = stream_read_forward(stream);
              use++;
              if (c<0x80) { // 0, 256, 128
                goto fail;
              } else {
                if (c<0xc0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  goto okay;
                } else {
                  goto fail;
                }
              }
            } else {
              goto fail;
            }
          } else {
            goto fail;
          }
        } else {
          cp = (uint8_t)(c&0x3f);
          uint8_t c = stream_read_forward(stream);
          use++;
          if (c<0x80) { // 0, 256, 128
            goto fail;
          } else {
            if (c<0xc0) { // 128, 256, 192
              cp <<= 6;
              cp |= (uint8_t)(c&0x7f);
              goto okay;
            } else {
              goto fail;
            }
          }
        }
      } else {
        if (c<0xf0) { // 225, 256, 240
          if (c>=0xed) { // 225, 240, 232
            if (c<0xee) { // 237, 240, 238
              cp = 0x0d;
              uint8_t c = stream_read_forward(stream);
              use++;
              if (c<0x80) { // 0, 256, 128
                goto fail;
              } else {
                if (c<0xa0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  uint8_t c = stream_read_forward(stream);
                  use++;
                  if (c<0x80) { // 0, 256, 128
                    goto fail;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      goto okay;
                    } else {
                      goto fail;
                    }
                  }
                } else {
                  goto fail;
                }
              }
            } else {
              cp = (uint8_t)(c&0x1f);
              uint8_t c = stream_read_forward(stream);
              use++;
              if (c<0x80) { // 0, 256, 128
                goto fail;
              } else {
                if (c<0xc0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  uint8_t c = stream_read_forward(stream);
                  use++;
                  if (c<0x80) { // 0, 256, 128
                    goto fail;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      goto okay;
                    } else {
                      goto fail;
                    }
                  }
                } else {
                  goto fail;
                }
              }
            }
          } else {
            cp = (uint8_t)(c&0x1f);
            uint8_t c = stream_read_forward(stream);
            use++;
            if (c<0x80) { // 0, 256, 128
              goto fail;
            } else {
              if (c<0xc0) { // 128, 256, 192
                cp <<= 6;
                cp |= (uint8_t)(c&0x7f);
                uint8_t c = stream_read_forward(stream);
                use++;
                if (c<0x80) { // 0, 256, 128
                  goto fail;
                } else {
                  if (c<0xc0) { // 128, 256, 192
                    cp <<= 6;
                    cp |= (uint8_t)(c&0x7f);
                    goto okay;
                  } else {
                    goto fail;
                  }
                }
              } else {
                goto fail;
              }
            }
          }
        } else {
          if (c<0xf5) { // 240, 256, 248
            if (c<0xf1) { // 240, 245, 242
              uint8_t c = stream_read_forward(stream);
              use++;
              if (c>=0x90) { // 0, 256, 128
                if (c<0xc0) { // 144, 256, 200
                  cp = (uint8_t)(c&0x7f);
                  uint8_t c = stream_read_forward(stream);
                  use++;
                  if (c<0x80) { // 0, 256, 128
                    goto fail;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      uint8_t c = stream_read_forward(stream);
                      use++;
                      if (c<0x80) { // 0, 256, 128
                        goto fail;
                      } else {
                        if (c<0xc0) { // 128, 256, 192
                          cp <<= 6;
                          cp |= (uint8_t)(c&0x7f);
                          goto okay;
                        } else {
                          goto fail;
                        }
                      }
                    } else {
                      goto fail;
                    }
                  }
                } else {
                  goto fail;
                }
              } else {
                goto fail;
              }
            } else {
              if (c>=0xf4) { // 241, 245, 243
                cp = 0x04;
                uint8_t c = stream_read_forward(stream);
                use++;
                if (c<0x80) { // 0, 256, 128
                  goto fail;
                } else {
                  if (c<0x81) { // 128, 256, 192
                    cp <<= 6;
                    uint8_t c = stream_read_forward(stream);
                    use++;
                    if (c<0x80) { // 0, 256, 128
                      goto fail;
                    } else {
                      if (c<0xc0) { // 128, 256, 192
                        cp <<= 6;
                        cp |= (uint8_t)(c&0x7f);
                        uint8_t c = stream_read_forward(stream);
                        use++;
                        if (c<0x80) { // 0, 256, 128
                          goto fail;
                        } else {
                          if (c<0xc0) { // 128, 256, 192
                            cp <<= 6;
                            cp |= (uint8_t)(c&0x7f);
                            goto okay;
                          } else {
                            goto fail;
                          }
                        }
                      } else {
                        goto fail;
                      }
                    }
                  } else {
                    goto fail;
                  }
                }
              } else {
                cp = (uint8_t)(c&0x0f);
                uint8_t c = stream_read_forward(stream);
                use++;
                if (c<0x80) { // 0, 256, 128
                  goto fail;
                } else {
                  if (c<0xc0) { // 128, 256, 192
                    cp <<= 6;
                    cp |= (uint8_t)(c&0x7f);
                    uint8_t c = stream_read_forward(stream);
                    use++;
                    if (c<0x80) { // 0, 256, 128
                      goto fail;
                    } else {
                      if (c<0xc0) { // 128, 256, 192
                        cp <<= 6;
                        cp |= (uint8_t)(c&0x7f);
                        uint8_t c = stream_read_forward(stream);
                        use++;
                        if (c<0x80) { // 0, 256, 128
                          goto fail;
                        } else {
                          if (c<0xc0) { // 128, 256, 192
                            cp <<= 6;
                            cp |= (uint8_t)(c&0x7f);
                            goto okay;
                          } else {
                            goto fail;
                          }
                        }
                      } else {
                        goto fail;
                      }
                    }
                  } else {
                    goto fail;
                  }
                }
              }
            }
          } else {
            goto fail;
          }
        }
      }
    } else {
      goto fail;
    }
  }

fail:;
  stream_reverse(stream, use-1);
  *used = 1;
  return UNICODE_CODEPOINT_BAD;
okay:;
  *used = use;
  return cp;
}

codepoint_t encoding_utf8_decode(struct encoding* base, struct stream* stream, size_t* used) {

  if (UNLIKELY(stream_left(stream)<4)) {
    return encoding_utf8_decode_boundary(base, stream, used);
  }

  codepoint_t cp;
  size_t use = 0;

  const uint8_t* buffer = stream_buffer(stream);
  uint8_t c = buffer[use++];
  PREFETCH(&buffer[use], 0, 0);
  if (LIKELY(c<0x80)) { // 0, 256, 128
    cp = c;
    goto okay2;
  } else {
    if (c>=0xc2) { // 128, 256, 192
      if (c<0xe1) { // 194, 256, 225
        if (c>=0xe0) { // 194, 225, 209
          uint8_t c = buffer[use++];
          if (c>=0xa0) { // 0, 256, 128
            if (c<0xc0) { // 160, 256, 208
              cp = (uint8_t)(c&0x7f);
              uint8_t c = buffer[use++];
              if (c<0x80) { // 0, 256, 128
                goto fail2;
              } else {
                if (c<0xc0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  goto okay2;
                } else {
                  goto fail2;
                }
              }
            } else {
              goto fail2;
            }
          } else {
            goto fail2;
          }
        } else {
          cp = (uint8_t)(c&0x3f);
          uint8_t c = buffer[use++];
          if (c<0x80) { // 0, 256, 128
            goto fail2;
          } else {
            if (c<0xc0) { // 128, 256, 192
              cp <<= 6;
              cp |= (uint8_t)(c&0x7f);
              goto okay2;
            } else {
              goto fail2;
            }
          }
        }
      } else {
        if (c<0xf0) { // 225, 256, 240
          if (c>=0xed) { // 225, 240, 232
            if (c<0xee) { // 237, 240, 238
              cp = 0x0d;
              uint8_t c = buffer[use++];
              if (c<0x80) { // 0, 256, 128
                goto fail2;
              } else {
                if (c<0xa0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  uint8_t c = buffer[use++];
                  if (c<0x80) { // 0, 256, 128
                    goto fail2;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      goto okay2;
                    } else {
                      goto fail2;
                    }
                  }
                } else {
                  goto fail2;
                }
              }
            } else {
              cp = (uint8_t)(c&0x1f);
              uint8_t c = buffer[use++];
              if (c<0x80) { // 0, 256, 128
                goto fail2;
              } else {
                if (c<0xc0) { // 128, 256, 192
                  cp <<= 6;
                  cp |= (uint8_t)(c&0x7f);
                  uint8_t c = buffer[use++];
                  if (c<0x80) { // 0, 256, 128
                    goto fail2;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      goto okay2;
                    } else {
                      goto fail2;
                    }
                  }
                } else {
                  goto fail2;
                }
              }
            }
          } else {
            cp = (uint8_t)(c&0x1f);
            uint8_t c = buffer[use++];
            if (c<0x80) { // 0, 256, 128
              goto fail2;
            } else {
              if (c<0xc0) { // 128, 256, 192
                cp <<= 6;
                cp |= (uint8_t)(c&0x7f);
                uint8_t c = buffer[use++];
                if (c<0x80) { // 0, 256, 128
                  goto fail2;
                } else {
                  if (c<0xc0) { // 128, 256, 192
                    cp <<= 6;
                    cp |= (uint8_t)(c&0x7f);
                    goto okay2;
                  } else {
                    goto fail2;
                  }
                }
              } else {
                goto fail2;
              }
            }
          }
        } else {
          if (c<0xf5) { // 240, 256, 248
            if (c<0xf1) { // 240, 245, 242
              uint8_t c = buffer[use++];
              if (c>=0x90) { // 0, 256, 128
                if (c<0xc0) { // 144, 256, 200
                  cp = (uint8_t)(c&0x7f);
                  uint8_t c = buffer[use++];
                  if (c<0x80) { // 0, 256, 128
                    goto fail2;
                  } else {
                    if (c<0xc0) { // 128, 256, 192
                      cp <<= 6;
                      cp |= (uint8_t)(c&0x7f);
                      uint8_t c = buffer[use++];
                      if (c<0x80) { // 0, 256, 128
                        goto fail2;
                      } else {
                        if (c<0xc0) { // 128, 256, 192
                          cp <<= 6;
                          cp |= (uint8_t)(c&0x7f);
                          goto okay2;
                        } else {
                          goto fail2;
                        }
                      }
                    } else {
                      goto fail2;
                    }
                  }
                } else {
                  goto fail2;
                }
              } else {
                goto fail2;
              }
            } else {
              if (c>=0xf4) { // 241, 245, 243
                cp = 0x04;
                uint8_t c = buffer[use++];
                if (c<0x80) { // 0, 256, 128
                  goto fail2;
                } else {
                  if (c<0x81) { // 128, 256, 192
                    cp <<= 6;
                    uint8_t c = buffer[use++];
                    if (c<0x80) { // 0, 256, 128
                      goto fail2;
                    } else {
                      if (c<0xc0) { // 128, 256, 192
                        cp <<= 6;
                        cp |= (uint8_t)(c&0x7f);
                        uint8_t c = buffer[use++];
                        if (c<0x80) { // 0, 256, 128
                          goto fail2;
                        } else {
                          if (c<0xc0) { // 128, 256, 192
                            cp <<= 6;
                            cp |= (uint8_t)(c&0x7f);
                            goto okay2;
                          } else {
                            goto fail2;
                          }
                        }
                      } else {
                        goto fail2;
                      }
                    }
                  } else {
                    goto fail2;
                  }
                }
              } else {
                cp = (uint8_t)(c&0x0f);
                uint8_t c = buffer[use++];
                if (c<0x80) { // 0, 256, 128
                  goto fail2;
                } else {
                  if (c<0xc0) { // 128, 256, 192
                    cp <<= 6;
                    cp |= (uint8_t)(c&0x7f);
                    uint8_t c = buffer[use++];
                    if (c<0x80) { // 0, 256, 128
                      goto fail2;
                    } else {
                      if (c<0xc0) { // 128, 256, 192
                        cp <<= 6;
                        cp |= (uint8_t)(c&0x7f);
                        uint8_t c = buffer[use++];
                        if (c<0x80) { // 0, 256, 128
                          goto fail2;
                        } else {
                          if (c<0xc0) { // 128, 256, 192
                            cp <<= 6;
                            cp |= (uint8_t)(c&0x7f);
                            goto okay2;
                          } else {
                            goto fail2;
                          }
                        }
                      } else {
                        goto fail2;
                      }
                    }
                  } else {
                    goto fail2;
                  }
                }
              }
            }
          } else {
            goto fail2;
          }
        }
      }
    } else {
      goto fail2;
    }
  }

fail2:;
  *used = 1;
  stream->displacement++;
  return UNICODE_CODEPOINT_BAD;
okay2:;
  *used = use;
  stream->displacement+=use;
  return cp;
}

size_t encoding_utf8_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  if (cp<0x80) {
    if (size<1) {
      return 0;
    }

    *text++ = (uint8_t)cp;
    return 1;
  } else if (cp<0x800) {
    if (size<2) {
      return 0;
    }

    *text++ = 0xc0+(uint8_t)(cp>>6);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 2;
  } else if (cp<0x10000) {
    if (size<3) {
      return 0;
    }

    *text++ = 0xe0+(uint8_t)(cp>>12);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 3;
  } else if (cp<0x101000) {
    if (size<4) {
      return 0;
    }

    *text++ = 0xf0+(uint8_t)(cp>>18);
    *text++ = 0x80+(uint8_t)((cp>>12)&0x3f);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 4;
  }

  return 0;
}

size_t encoding_utf8_next(struct encoding* base, struct stream* stream) {
  uint8_t c = stream_read_forward(stream);
  if ((c&0x80)==0) {
    return 1;
  }

  if ((c&0xe0)==0xc0) {
    if ((c&0x1f)<2) {
      return 1;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    return 2;
  } else if ((c&0xf0)==0xe0) {
    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      return 1;
    }

    return 3;
  } else if ((c&0xf8)==0xf0) {
    if (c>0xf4) {
      return 1;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      return 1;
    }
    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      return 1;
    }

    uint8_t c3 = stream_read_forward(stream);
    if ((c3&0xc0)!=0x80) {
      stream_reverse(stream, 3);
      return 1;
    }

    return 4;
  }

  return 1;
}

struct utf8_table {
  struct {
    int add;
    size_t next;
    int left;
    bool_t end;
    bool_t bad;
  } jump[256];

  size_t min;
  size_t max;
  size_t rename;
  bool_t used;
};

void code_depth(int depth) {
  for (int n = 0; n<depth; n++) {
    fprintf(stderr, "  ");
  }
}

void code_table(struct utf8_table* tables, size_t table, size_t from, size_t to, int depth, int read, int first) {

  if (read) {
    code_depth(depth);
    fprintf(stderr, "uint8_t c = stream_read_forward(stream);\r\n");
    code_depth(depth);
    fprintf(stderr, "use++;\r\n");
  }

  size_t mid = (from+to)/2;
  bool_t outer = 0;
  if (mid!=from) {
    // Bad enclosing
    /*{
      size_t begin = to;
      for (size_t dist = from; dist<to; dist++) {
        if (!tables[table].jump[dist].bad) {
          begin = dist;
          break;
        }
      }

      if (begin+1<to) {
        size_t stop = to;
        for (size_t dist = begin+1; dist<to; dist++) {
          if (tables[table].jump[begin].next!=tables[table].jump[dist].next || tables[table].jump[begin].end!=tables[table].jump[dist].end || tables[table].jump[dist].bad) {
            stop = dist;
            break;
          }
        }

        size_t end = to;
        for (size_t dist = stop+1; dist<to; dist++) {
          if (!tables[table].jump[dist].bad) {
            end = dist;
            break;
          }
        }
        if (end==to && (begin!=from || stop!=to) && begin+1<stop) {
          size_t size = stop-begin;
          size_t mask = (size-1);
          if ((mask&size)==0) {
            if ((begin&mask)==0) {
              code_depth(depth);
              //fprintf(stderr, "// enclosing ya %02x %02x\r\n", (int)((uint8_t)~mask), (int)(begin&~mask));
              fprintf(stderr, "if ((c&0x%02x)==0x%02x) {\r\n", (int)((uint8_t)~mask), (int)(begin&~mask));
              code_table(tables, table, begin, stop, depth+1, 0, first);
              code_depth(depth);
              fprintf(stderr, "} else {\r\n");
              if (begin!=from && end!=to) {
                depth++;
                code_depth(depth);
                fprintf(stderr, "if (c<0x%02x) {\r\n", (int)begin);
                code_table(tables, table, from, begin, depth+1, 0, first);
                code_depth(depth);
                fprintf(stderr, "} else {\r\n");
                code_table(tables, table, stop, to, depth+1, 0, first);
                code_depth(depth);
                fprintf(stderr, "}\r\n");
                depth--;
              } else if (begin!=from) {
                code_table(tables, table, from, begin, depth+1, 0, first);
              } else {
                code_table(tables, table, stop, to, depth+1, 0, first);
              }
              code_depth(depth);
              fprintf(stderr, "}\r\n");
              outer = 1;
            }
          }
        }
      }
    }*/

    if (!outer) {
      // Range divider
      for (size_t dist = 0; dist<((to-from)/2)+2; dist++) {
        if (mid-from>=dist && ( tables[table].jump[mid-dist].next!=tables[table].jump[mid].next || tables[table].jump[mid-dist].end!=tables[table].jump[mid].end || tables[table].jump[mid-dist].bad!=tables[table].jump[mid].bad)) {
          code_depth(depth);
          fprintf(stderr, "if (c<0x%02x) { // %d, %d, %d\r\n", (int)(mid-dist+1), (int)from, (int)to, (int)mid);
          code_table(tables, table, from, mid-dist+1, depth+1, 0, first);
          code_depth(depth);
          fprintf(stderr, "} else {\r\n");
          code_table(tables, table, mid-dist+1, to, depth+1, 0, first);
          code_depth(depth);
          fprintf(stderr, "}\r\n");
          outer = 1;
          break;
        } else if (to-mid>dist && ( tables[table].jump[mid+dist].next!=tables[table].jump[mid].next || tables[table].jump[mid+dist].end!=tables[table].jump[mid].end || tables[table].jump[mid+dist].bad!=tables[table].jump[mid].bad)) {
          code_depth(depth);
          fprintf(stderr, "if (c>=0x%02x) { // %d, %d, %d\r\n", (int)(mid+dist), (int)from, (int)to, (int)mid);
          code_table(tables, table, mid+dist, to, depth+1, 0, first);
          code_depth(depth);
          fprintf(stderr, "} else {\r\n");
          code_table(tables, table, from, mid+dist, depth+1, 0, first);
          code_depth(depth);
          fprintf(stderr, "}\r\n");
          outer = 1;
          break;
        }
      }
    }
  }

  if (!outer) {
    if (tables[table].jump[from].bad) {
      code_depth(depth);
      fprintf(stderr, "goto fail;\r\n");
    } else {
      if (!first) {
        code_depth(depth);
        fprintf(stderr, "cp <<= 6;\r\n");
      }

      if (mid==from) {
        if (tables[table].jump[from].add!=0) {
          code_depth(depth);
          fprintf(stderr, "cp %s= 0x%02x;\r\n", first?"":"|", (int)tables[table].jump[from].add);
          first = 0;
        }
      } else {
        code_depth(depth);
        int add = (int)((int)tables[table].jump[from].add-(int)from)&0xff;
        if (add!=0) {
          if ((add&(add-1))==0) {
            fprintf(stderr, "cp %s= (uint8_t)(c&0x%02x);\r\n", first?"":"|", add-1);
          } else {
            fprintf(stderr, "cp %s= (uint8_t)(c+0x%02x);\r\n", first?"":"|", add);
          }
        } else {
          fprintf(stderr, "cp %s= c;\r\n", first?"":"|");
        }

        first = 0;
      }

      if (!tables[table].jump[from].end) {
        code_table(tables, tables[table].jump[from].next, 0, 256, depth, 1, first);
      } else {
        code_depth(depth);
        fprintf(stderr, "goto okay;\r\n");
      }
    }
  }
}

void encoding_utf8_build_tables(void) {
  size_t size = 0x10000;
  struct utf8_table* tables = (struct utf8_table*)malloc(sizeof(struct utf8_table)*size);
  memset(tables, 0, sizeof(struct utf8_table)*size);
  for (size_t r = 0; r<size; r++) {
    for (size_t v = 0; v<256; v++) {
      tables[r].jump[v].end = 1;
      tables[r].jump[v].bad = 1;
    }
  }

  size_t table = 1;
  for (codepoint_t cp = 0; cp<0x110000; cp++) {
    if (cp<0xd800 || cp>=0xe000) {
      uint8_t out[8];
      size_t length = encoding_utf8_encode(NULL, cp, &out[0], 8);

      size_t current = 0;
      size_t pos = 0;
      while (pos<length) {
        if (tables[current].jump[out[pos]].next==0 && pos+1<length) {
          tables[current].jump[out[pos]].next = table++;
          tables[current].jump[out[pos]].end = 0;
        }

        int add = out[pos];
        if (pos==0 && length==1) {
          add &= 0x7f;
        } else if (pos==0 && length==2) {
          add &= 0x1f;
        } else if (pos==0 && length==3) {
          add &= 0x0f;
        } else if (pos==0 && length==4) {
          add &= 0x07;
        } else if (pos==0 && length==5) {
          add &= 0x03;
        } else {
          add &= 0x3f;
        }

        //for (size_t k = pos+1; k<length; k++) {
        //  add<<=6;
        //}
        tables[current].jump[out[pos]].add = add;
        tables[current].jump[out[pos]].bad = 0;
        tables[current].jump[out[pos]].left = (int)(length-1-pos);
        current = tables[current].jump[out[pos]].next;
        pos++;
      }
    }
  }

  fprintf(stderr, "tables: %d\r\n", (int)table);

  for (size_t r = 0; r<table; r++) {
    size_t min = 0x1000000;
    size_t max = 0;
    for (size_t v = 0; v<256; v++) {
      if (tables[r].jump[v].next<min) {
        min = tables[r].jump[v].next;
      }
      if (tables[r].jump[v].next>max) {
        max = tables[r].jump[v].next;
      }
    }
    tables[r].min = min;
    tables[r].max = max;
  }

  while (1) {
    bool_t use = 0;
    for (size_t n = 0; n<table; n++) {
      if (!tables[n].used) {
        for (size_t m = n+1; m<table; m++) {
          if (!tables[m].used) {
            size_t v = 0;
            for (; v<256; v++) {
              if (tables[n].jump[v].add!=tables[m].jump[v].add || tables[n].jump[v].next!=tables[m].jump[v].next || tables[n].jump[v].left!=tables[m].jump[v].left) {
                break;
              }
            }

            if (v==256) {
              use = 1;
              tables[m].used = 1;
              fprintf(stderr, "reduce %d/%d\r\n", (int)n, (int)m);
              for (size_t r = 0; r<table; r++) {
                if (!tables[r].used && tables[r].min<=m && tables[r].max>=m) {
                  size_t min = 0x1000000;
                  size_t max = 0;
                  for (size_t v = 0; v<256; v++) {
                    if (tables[r].jump[v].next==m) {
                      tables[r].jump[v].next = n;
                    }
                    if (tables[r].jump[v].next<min) {
                      min = tables[r].jump[v].next;
                    }
                    if (tables[r].jump[v].next>max) {
                      max = tables[r].jump[v].next;
                    }
                  }
                  tables[r].min = min;
                  tables[r].max = max;
                }
              }
            }
          }
        }
        //fprintf(stderr, "\rtab: %d\r\n", (int)n);
      }
    }
    if (!use) {
      break;
    }
  }

  size_t reduce = 0;
  for (size_t n = 0; n<table; n++) {
    if (!tables[n].used) {
      tables[n].rename = reduce;
      reduce++;
    }
  }

  fprintf(stderr, "reduced: %d\r\n", (int)reduce);

  for (size_t n = 0; n<table; n++) {
    if (!tables[n].used) {
      //fprintf(stderr, "> Table %d (%d)\r\n", (int)tables[n].rename, (int)n);
      for (size_t v = 0; v<256; v++) {
        fprintf(stderr, "0x%04x|%d%d, ", (int)(tables[n].jump[v].add)|
          (int)(tables[tables[n].jump[v].next].rename<<8), (int)tables[n].jump[v].end, (int)tables[n].jump[v].bad);
        if ((v&7)==7) {
          fprintf(stderr, "\r\n");
        } else {
          fprintf(stderr, " ");
        }
      }
    }
  }

  code_table(tables, 0, 0, 256, 0, 1, 1);
}
