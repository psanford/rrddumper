/****************************************************************************
 * RRDtool 1.1.x  Copyright Tobias Oetiker, 1997 - 2002
 ****************************************************************************
 * rrd_gfx.c  graphics wrapper for rrdtool
  **************************************************************************/

/* #define DEBUG */

#ifdef DEBUG
# define DPRINT(x)    (void)(printf x, printf("\n"))
#else
# define DPRINT(x)
#endif

#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>

#include "rrd_gfx.h"

/* lines are better drawn on the pixle than between pixles */
#define LINEOFFSET 0.5

static
gfx_node_t *gfx_new_node( gfx_canvas_t *canvas,enum gfx_en type){
  gfx_node_t *node = art_new(gfx_node_t,1);
  if (node == NULL) return NULL;
  node->type = type;
  node->color = 0x0;        /* color of element  0xRRGGBBAA  alpha 0xff is solid*/
  node->size =0.0;         /* font size, line width */
  node->path = NULL;        /* path */
  node->points = 0;
  node->points_max =0;
  node->svp = NULL;         /* svp */
  node->filename = NULL;             /* font or image filename */
  node->text = NULL;
  node->x = 0.0;
  node->y = 0.0;          /* position */
  node->angle = 0;  
  node->halign = GFX_H_NULL; /* text alignement */
  node->valign = GFX_V_NULL; /* text alignement */
  node->tabwidth = 0.0; 
  node->next = NULL; 
  if (canvas->lastnode != NULL){
      canvas->lastnode->next = node;
  }
  if (canvas->firstnode == NULL){
      canvas->firstnode = node;
  }  
  canvas->lastnode = node;
  return node;
}

gfx_canvas_t *gfx_new_canvas (void) {
    gfx_canvas_t *canvas = art_new(gfx_canvas_t,1);
    canvas->firstnode = NULL;
    canvas->lastnode = NULL;
    return canvas;    
}

/* create a new line */
gfx_node_t  *gfx_new_line(gfx_canvas_t *canvas, 
			   double x0, double y0, 
	 		   double x1, double y1,
 			   double width, gfx_color_t color){

  gfx_node_t *node;
  ArtVpath *vec;
  node = gfx_new_node(canvas,GFX_LINE);
  if (node == NULL) return NULL;
  vec = art_new(ArtVpath, 3);
  if (vec == NULL) return NULL;
  vec[0].code = ART_MOVETO_OPEN; vec[0].x=x0+LINEOFFSET; vec[0].y=y0+LINEOFFSET;
  vec[1].code = ART_LINETO; vec[1].x=x1+LINEOFFSET; vec[1].y=y1+LINEOFFSET;
  vec[2].code = ART_END;
  
  node->points = 3;
  node->points_max = 3;
  node->color = color;
  node->size  = width;
  node->path  = vec;
  return node;
}

/* create a new area */
gfx_node_t   *gfx_new_area   (gfx_canvas_t *canvas, 
			      double x0, double y0,
			      double x1, double y1,
			      double x2, double y2,
			      gfx_color_t color) {

  gfx_node_t *node;
  ArtVpath *vec;
  node = gfx_new_node(canvas,GFX_AREA);
  if (node == NULL) return NULL;
  vec = art_new(ArtVpath, 5);
  if (vec == NULL) return NULL;
  vec[0].code = ART_MOVETO; vec[0].x=x0; vec[0].y=y0;
  vec[1].code = ART_LINETO; vec[1].x=x1; vec[1].y=y1;
  vec[2].code = ART_LINETO; vec[2].x=x2; vec[2].y=y2;
  vec[3].code = ART_LINETO; vec[3].x=x0; vec[3].y=y0;
  vec[4].code = ART_END;
  
  node->points = 5;
  node->points_max = 5;
  node->color = color;
  node->path  = vec;

  return node;
}

/* add a point to a line or to an area */
int           gfx_add_point  (gfx_node_t *node, 
			      double x, double y){
  if (node == NULL) return 1;
  if (node->type == GFX_AREA) {
    double x0 = node->path[0].x;
    double y0 = node->path[0].y;
    node->points -= 2;
    art_vpath_add_point (&(node->path),
                         &(node->points),
                         &(node->points_max),
                         ART_LINETO,
                         x,y);
    art_vpath_add_point (&(node->path),
                         &(node->points),
                         &(node->points_max),
                         ART_LINETO,
                         x0,y0);
    art_vpath_add_point (&(node->path),
                         &(node->points),
                         &(node->points_max),
                         ART_END,
                         0,0);
  } else if (node->type == GFX_LINE) {
    node->points -= 1;
    art_vpath_add_point (&(node->path),
                         &(node->points),
                         &(node->points_max),
                         ART_LINETO,
                         x+LINEOFFSET,y+LINEOFFSET);
    art_vpath_add_point (&(node->path),
                         &(node->points),
                         &(node->points_max),
                         ART_END,
                         0,0);
    
  } else {
    /* can only add point to areas and lines */
    return 1;
  }
  return 0;
}



/* create a text node */
gfx_node_t   *gfx_new_text   (gfx_canvas_t *canvas,  
			      double x, double y, gfx_color_t color,
			      char* font, double size, 			      
			      double tabwidth, double angle,
			      enum gfx_h_align_en h_align,
			      enum gfx_v_align_en v_align,
                              char* text){
   gfx_node_t *node = gfx_new_node(canvas,GFX_TEXT);
   if (angle != 0.0){
       /* currently we only support 0 and 270 */
       angle = 270.0;
   }
   
   node->text = strdup(text);
   node->size = size;
   node->filename = strdup(font);
   node->x = x;
   node->y = y;
   node->angle = angle;   
   node->color = color;
   node->tabwidth = tabwidth;
   node->halign = h_align;
   node->valign = v_align;
   return node;
}

double gfx_get_text_width ( double start, char* font, double size, 			      
			    double tabwidth, char* text){

  FT_GlyphSlot  slot;
  FT_UInt       previous=0;
  FT_UInt       glyph_index=0;
  FT_Bool       use_kerning;
  int           error;
  FT_Face       face;
  FT_Library    library=NULL;  
  double        text_width=0;
  FT_Init_FreeType( &library );
  error = FT_New_Face( library, font, 0, &face );
  if ( error ) return -1;
  error = FT_Set_Char_Size(face,  size*64,size*64,  100,100);
  if ( error ) return -1;

  use_kerning = FT_HAS_KERNING(face);
  slot = face->glyph;
  for(;*text;text++) {	
    previous = glyph_index;
    glyph_index = FT_Get_Char_Index( face, *text);
    
    if (use_kerning && previous && glyph_index){
      FT_Vector  delta;
      FT_Get_Kerning( face, previous, glyph_index,
		      0, &delta );
      text_width += (double)delta.x / 64.0;
      
    }
    error = FT_Load_Glyph( face, glyph_index, 0 );
    if ( error ) {
      FT_Done_FreeType(library);
      return -1;
    }
    if (! previous) {
      text_width -= (double)slot->metrics.horiBearingX / 64.0; /* add just char width */	
    }
    text_width += (double)slot->metrics.horiAdvance / 64.0;
  }
  text_width -= (double)slot->metrics.horiAdvance / 64.0; /* remove last step */
  text_width += (double)slot->metrics.width / 64.0; /* add just char width */
  text_width += (double)slot->metrics.horiBearingX / 64.0; /* add just char width */
  FT_Done_FreeType(library);
  return text_width;
}
 



static int gfx_save_png (art_u8 *buffer, FILE *fp,
                     long width, long height, long bytes_per_pixel);
/* render grafics into png image */
int           gfx_render_png (gfx_canvas_t *canvas, 
			      art_u32 width, art_u32 height, 
		              double zoom, 
			      gfx_color_t background, FILE *fp){
    
    
    FT_Library    library;
    gfx_node_t *node = canvas->firstnode;    
    art_u8 red = background >> 24, green = (background >> 16) & 0xff;
    art_u8 blue = (background >> 8) & 0xff, alpha = ( background & 0xff );
    unsigned long pys_width = width * zoom;
    unsigned long pys_height = height * zoom;
    const int bytes_per_pixel = 3;
    unsigned long rowstride = pys_width*bytes_per_pixel; /* bytes per pixel */
    art_u8 *buffer = art_new (art_u8, rowstride*pys_height);
    art_rgb_run_alpha (buffer, red, green, blue, alpha, pys_width*pys_height);
    FT_Init_FreeType( &library );
    while(node){
        switch (node->type) {
        case GFX_LINE:
        case GFX_AREA: {   
            ArtVpath *vec;
            double dst[6];     
            ArtSVP *svp;
            art_affine_scale(dst,zoom,zoom);
            vec = art_vpath_affine_transform(node->path,dst);
            if(node->type == GFX_LINE){
                svp = art_svp_vpath_stroke ( vec, ART_PATH_STROKE_JOIN_ROUND,
                                             ART_PATH_STROKE_CAP_ROUND,
                                             node->size*zoom,1,1);
            } else {
                svp = art_svp_from_vpath ( vec );
            }
            art_free(vec);
            art_rgb_svp_alpha (svp ,0,0, pys_width, pys_height,
                               node->color, buffer, rowstride, NULL);
            art_free(svp);
            break;
        }
        case GFX_TEXT: {
            int  error;
            float text_width=0.0, text_height = 0.0;
            unsigned char *text;
            art_u8 fcolor[3],falpha;
            FT_Face       face;
            FT_GlyphSlot  slot;
            FT_UInt       previous=0;
            FT_UInt       glyph_index=0;
	    FT_Bool       use_kerning;

            float pen_x = 0.0 , pen_y = 0.0;
            /* double x,y; */
            long   ix,iy,iz;
            
            fcolor[0] = node->color >> 24;
            fcolor[1] = (node->color >> 16) & 0xff;
            fcolor[2] = (node->color >> 8) & 0xff;
            falpha = node->color & 0xff;
            error = FT_New_Face( library,
                                 (char *)node->filename,
                                 0,
                                 &face );
	    if ( error ) break;
            use_kerning = FT_HAS_KERNING(face);

            error = FT_Set_Char_Size(face,   /* handle to face object            */
                                     (long)(node->size*64),
                                     (long)(node->size*64),
                                     (long)(100*zoom),
                                     (long)(100*zoom));
            if ( error ) break;
            pen_x = node->x * zoom;
            pen_y = node->y * zoom;
            slot = face->glyph;

            for(text=(unsigned char *)node->text;*text;text++) {	
                previous = glyph_index;
                glyph_index = FT_Get_Char_Index( face, *text);
                
                if (use_kerning && previous && glyph_index){
                    FT_Vector  delta;
                    FT_Get_Kerning( face, previous, glyph_index,
                                    0, &delta );
                    text_width += (double)delta.x / 64.0;
                    
                }
                error = FT_Load_Glyph( face, glyph_index, 0 );
                if ( error ) break;
		if (previous == 0){
		  pen_x -= (double)slot->metrics.horiBearingX / 64.0; /* adjust pos for first char */	
		  text_width -= (double)slot->metrics.horiBearingX / 64.0; /* add just char width */	
                }
		if ( text_height < (double)slot->metrics.horiBearingY / 64.0 ) {
		  text_height = (double)slot->metrics.horiBearingY / 64.0;
		}
                text_width += (double)slot->metrics.horiAdvance / 64.0;
            }
            text_width -= (double)slot->metrics.horiAdvance / 64.0; /* remove last step */
            text_width += (double)slot->metrics.width / 64.0; /* add just char width */
            text_width += (double)slot->metrics.horiBearingX / 64.0; /* add just char width */
            
            switch(node->halign){
            case GFX_H_RIGHT:  pen_x -= text_width; break;
            case GFX_H_CENTER: pen_x -= text_width / 2.0; break;          
            case GFX_H_LEFT: break;          
            }

            switch(node->valign){
            case GFX_V_TOP:    pen_y += text_height; break;
            case GFX_V_CENTER: pen_y += text_height / 2.0; break;          
            case GFX_V_BOTTOM: break;          
            }

            glyph_index=0;
            for(text=(unsigned char *)node->text;*text;text++) {
                int gr;          
                previous = glyph_index;
                glyph_index = FT_Get_Char_Index( face, *text);
                
                if (use_kerning && previous && glyph_index){
                    FT_Vector  delta;
                    FT_Get_Kerning( face, previous, glyph_index,
                                    0, &delta );
                    pen_x += (double)delta.x / 64.0;
                    
                }
                error = FT_Load_Glyph( face, glyph_index, FT_LOAD_RENDER );
                if ( error ) break;
                gr = slot->bitmap.num_grays -1;
                for (iy=0; iy < slot->bitmap.rows; iy++){
                    long buf_y = iy+(pen_y+0.5)-slot->bitmap_top;
                    if (buf_y < 0 || buf_y >= pys_height) continue;
                    buf_y *= rowstride;
                    for (ix=0;ix < slot->bitmap.width;ix++){
                        long buf_x = ix + (pen_x + 0.5) + (double)slot->bitmap_left ;
                        art_u8 font_alpha;
                        
                        if (buf_x < 0 || buf_x >= pys_width) continue;
                        buf_x *=  bytes_per_pixel ;
                        font_alpha =  *(slot->bitmap.buffer + iy * slot->bitmap.width + ix);
                        font_alpha =  (art_u8)((double)font_alpha / gr * falpha);
                        for (iz = 0; iz < 3; iz++){
                            art_u8 *orig = buffer + buf_y + buf_x + iz;
                            *orig =  (art_u8)((double)*orig / gr * ( gr - font_alpha) +
                                              (double)fcolor[iz] / gr * (font_alpha));
                        }
                    }
                }
                pen_x += (double)slot->metrics.horiAdvance / 64.0;
            }
        }
        }
        node = node->next;
    }  
    gfx_save_png(buffer,fp , pys_width,pys_height,bytes_per_pixel);
    art_free(buffer);
    FT_Done_FreeType( library );
    return 0;    
}

/* free memory used by nodes this will also remove memory required for
   associated paths and svcs ... but not for text strings */
int
gfx_destroy    (gfx_canvas_t *canvas){  
  gfx_node_t *next,*node = canvas->firstnode;
  while(node){
    next = node->next;
    art_free(node->path);
    art_free(node->svp);
    free(node->text);
    free(node->filename);
    art_free(node);
    node = next;
  }
  return 0;
}
 
static int gfx_save_png (art_u8 *buffer, FILE *fp,  long width, long height, long bytes_per_pixel){
  png_structp png_ptr = NULL;
  png_infop   info_ptr = NULL;
  int i;
  png_bytep *row_pointers;
  int rowstride = width * bytes_per_pixel;
  png_text text[2];
  
  if (fp == NULL)
    return (1);

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (png_ptr == NULL)
   {
      return (1);
   }
   row_pointers = (png_bytepp)png_malloc(png_ptr,
                                     height*sizeof(png_bytep));

  info_ptr = png_create_info_struct(png_ptr);

  if (info_ptr == NULL)
    {
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return (1);
    }

  if (setjmp(png_jmpbuf(png_ptr)))
    {
      /* If we get here, we had a problem writing the file */
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (1);
    }

  png_init_io(png_ptr, fp);
  png_set_IHDR (png_ptr, info_ptr,width, height,
                8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);

  text[0].key = "Software";
  text[0].text = "RRDtool, Tobias Oetiker <tobi@oetike.ch>, http://tobi.oetiker.ch";
  text[0].compression = PNG_TEXT_COMPRESSION_NONE;
  png_set_text (png_ptr, info_ptr, text, 1);

  /* Write header data */
  png_write_info (png_ptr, info_ptr);

  for (i = 0; i < height; i++)
    row_pointers[i] = (png_bytep) (buffer + i*rowstride);

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  return 1;
}

 
static int svg_indent = 0;
static int svg_single_line = 0;
static void svg_print_indent(FILE *fp)
{
  int i;
   for (i = svg_indent - svg_single_line; i > 0; i--) {
     putc(' ', fp);
     putc(' ', fp);
   }
}
 
static void svg_start_tag(FILE *fp, const char *name)
 {
   svg_print_indent(fp);
   putc('<', fp);
   fputs(name, fp);
   svg_indent++;
 }
 
 static void svg_close_tag_single_line(FILE *fp)
 {
   svg_single_line++;
   putc('>', fp);
 }
 
 static void svg_close_tag(FILE *fp)
 {
   putc('>', fp);
   if (!svg_single_line)
     putc('\n', fp);
 }
 
 static void svg_end_tag(FILE *fp, const char *name)
 {
   /* name is NULL if closing empty-node tag */
   svg_indent--;
   if (svg_single_line)
     svg_single_line--;
   else if (name)
     svg_print_indent(fp);
   if (name != NULL) {
     fputs("</", fp);
     fputs(name, fp);
   } else {
     putc('/', fp);
   }
   svg_close_tag(fp);
 }
 
 static void svg_close_tag_empty_node(FILE *fp)
 {
   svg_end_tag(fp, NULL);
 }
 
 static void svg_write_text(FILE *fp, const char *p)
 {
   char ch;
   const char *start, *last;
   if (!p)
     return;
   /* trim leading spaces */
   while (*p == ' ')
     p++;
   start = p;
   /* trim trailing spaces */
   last = p - 1;
   while ((ch = *p) != 0) {
     if (ch != ' ')
       last = p;
     p++;
  }
   /* encode trimmed text */
   p = start;
   while (p <= last) {
     ch = *p++;
     switch (ch) {
       case '&': fputs("&amp;", fp); break;
       case '<': fputs("&lt;", fp); break;
       case '>': fputs("&gt;", fp); break;
       case '"': fputs("&quot;", fp); break;
       default: putc(ch, fp);
     }
   }
 }
 
 static void svg_write_number(FILE *fp, double d)
 {
   /* omit decimals if integer to reduce filesize */
   char buf[60], *p;
   snprintf(buf, sizeof(buf), "%.2f", d);
   p = buf; /* doesn't trust snprintf return value */
   while (*p)
     p++;
   while (--p > buf) {
     char ch = *p;
     if (ch == '0') {
       *p = '\0'; /* zap trailing zeros */
       continue;
     }
     if (ch == '.')
       *p = '\0'; /* zap trailing dot */
     break;
   }
   fputs(buf, fp);
 }
 
 static int svg_color_is_black(int c)
 {
   /* gfx_color_t is RRGGBBAA, svg can use #RRGGBB like html */
   c = (int)((c >> 8) & 0xFFFFFF);
   return !c;
 }
 
 static void svg_write_color(FILE *fp, int c)
 {
   /* gfx_color_t is RRGGBBAA, svg can use #RRGGBB like html */
   c = (int)((c >> 8) & 0xFFFFFF);
   if ((c & 0x0F0F0F) == ((c >> 4) & 0x0F0F0F)) {
     /* css2 short form, #rgb is #rrggbb, not #r0g0b0 */
     fprintf(fp, "#%03X",
           ( ((c >> 8) & 0xF00)
           | ((c >> 4) & 0x0F0)
           | ( c       & 0x00F)));
   } else {
     fprintf(fp, "#%06X", c);
   }
 }
 
 static int svg_is_int_step(double a, double b)
 {
   double diff = fabs(a - b);
   return floor(diff) == diff;
 }
 
 static int svg_path_straight_segment(FILE *fp,
     double lastA, double currentA, double currentB,
     gfx_node_t *node,
     int segment_idx, int isx, char absChar, char relChar)
 {
   if (!svg_is_int_step(lastA, currentA)) {
     putc(absChar, fp);
     svg_write_number(fp, currentA);
     return 0;
   }
   if (segment_idx < node->points - 1) {
     ArtVpath *vec = node->path + segment_idx + 1;
     if (vec->code == ART_LINETO) {
       double nextA = (isx ? vec->x : vec->y) - LINEOFFSET;
       double nextB = (isx ? vec->y : vec->x) - LINEOFFSET;
       if (nextB == currentB
           && ((currentA >= lastA) == (nextA >= currentA))
           && svg_is_int_step(currentA, nextA)) {
         return 1; /* skip to next as it is a straight line  */
       }
     }
   }
   putc(relChar, fp);
   svg_write_number(fp, currentA - lastA);
   return 0;
 }
 
 static void svg_path(FILE *fp, gfx_node_t *node, int multi)
 {
   int i;
   double lastX = 0, lastY = 0;
   /* for straight lines <path..> tags take less space than
      <line..> tags because of the efficient packing
      in the 'd' attribute */
   svg_start_tag(fp, "path");
   if (!multi) {
     fputs(" stroke-width=\"", fp);
     svg_write_number(fp, node->size);
     fputs("\" stroke=\"", fp);
     svg_write_color(fp, node->color);
     fputs("\" fill=\"none\"", fp);
   }
   fputs(" d=\"", fp);
   /* specification of the 'd' attribute: */
   /* http://www.w3.org/TR/SVG/paths.html#PathDataGeneralInformation */
   for (i = 0; i < node->points; i++) {
     ArtVpath *vec = node->path + i;
     double x = vec->x - LINEOFFSET;
     double y = vec->y - LINEOFFSET;
     switch (vec->code) {
     case ART_MOVETO_OPEN: /* fall-through */
     case ART_MOVETO:
       putc('M', fp);
       svg_write_number(fp, x);
       putc(',', fp);
       svg_write_number(fp, y);
       break;
     case ART_LINETO:
       /* try optimize filesize by using minimal lineto commands */
       /* without introducing rounding errors. */
       if (x == lastX) {
         if (svg_path_straight_segment(fp, lastY, y, x, node, i, 0, 'V', 'v'))
           continue;
       } else if (y == lastY) {
         if (svg_path_straight_segment(fp, lastX, x, y, node, i, 1, 'H', 'h'))
           continue;
       } else {
         putc('L', fp);
         svg_write_number(fp, x);
         putc(',', fp);
         svg_write_number(fp, y);
       }
       break;
     case ART_CURVETO: break; /* unsupported */
     case ART_END: break; /* nop */
     }
     lastX = x;
     lastY = y;
   }
   fputs("\"", fp);
   svg_close_tag_empty_node(fp);
 }
 
 static void svg_multi_path(FILE *fp, gfx_node_t **nodeR)
 {
   /* optimize for multiple paths with the same color, penwidth, etc. */
   int num = 1;
   gfx_node_t *node = *nodeR;
   gfx_node_t *next = node->next;
   while (next) {
     if (next->type != node->type
         || next->size != node->size
         || next->color != node->color)
       break;
     next = next->next;
     num++;
   }
   if (num == 1) {
     svg_path(fp, node, 0);
     return;
   }
   svg_start_tag(fp, "g");
   fputs(" stroke-width=\"", fp);
   svg_write_number(fp, node->size);
   fputs("\" stroke=\"", fp);
   svg_write_color(fp, node->color);
   fputs("\" fill=\"none\"", fp);
   svg_close_tag(fp);
   while (num && node) {
     svg_path(fp, node, 1);
     if (!--num)
       break;
     node = node->next;
     *nodeR = node;
   }
   svg_end_tag(fp, "g");
 }
 
 static void svg_area(FILE *fp, gfx_node_t *node)
 {
   int i;
   double startX = 0, startY = 0;
   svg_start_tag(fp, "polygon");
   fputs(" fill=\"", fp);
   svg_write_color(fp, node->color);
   fputs("\" points=\"", fp);
   for (i = 0; i < node->points; i++) {
     ArtVpath *vec = node->path + i;
     double x = vec->x - LINEOFFSET;
     double y = vec->y - LINEOFFSET;
     switch (vec->code) {
       case ART_MOVETO_OPEN: /* fall-through */
       case ART_MOVETO:
         svg_write_number(fp, x);
         putc(',', fp);
         svg_write_number(fp, y);
         startX = x;
         startY = y;
         break;
       case ART_LINETO:
         if (i == node->points - 2
 			&& node->path[i + 1].code == ART_END
             && fabs(x - startX) < 0.001 && fabs(y - startY) < 0.001) {
           break; /* poly area always closed, no need for last point */
         }
         putc(' ', fp);
         svg_write_number(fp, x);
         putc(',', fp);
         svg_write_number(fp, y);
         break;
       case ART_CURVETO: break; /* unsupported */
       case ART_END: break; /* nop */
     }
   }
   fputs("\"", fp);
   svg_close_tag_empty_node(fp);
 }
 
 static void svg_text(FILE *fp, gfx_node_t *node)
 {
   double x = node->x - LINEOFFSET;
   double y = node->y - LINEOFFSET;
   if (node->angle != 0) {
     svg_start_tag(fp, "g");
     fputs(" transform=\"translate(", fp);
     svg_write_number(fp, x);
     fputs(",", fp);
     svg_write_number(fp, y);
     fputs(") rotate(", fp);
     svg_write_number(fp, node->angle);
     fputs(")\"", fp);
     x = y = 0;
     svg_close_tag(fp);
   }
   switch (node->valign) {
   case GFX_V_TOP:  y += node->size; break;
   case GFX_V_CENTER: y += node->size / 2; break;
   case GFX_V_BOTTOM: break;
   case GFX_V_NULL: break;
   }
   svg_start_tag(fp, "text");
   fputs(" x=\"", fp);
   svg_write_number(fp, x);
   fputs("\" y=\"", fp);
   svg_write_number(fp, y);
   fputs("\" font-size=\"", fp);
   svg_write_number(fp, node->size);
   fputs("\"", fp);
   if (!svg_color_is_black(node->color)) {
     fputs(" fill=\"", fp);
     svg_write_color(fp, node->color);
     fputs("\"", fp);
   }
   switch (node->halign) {
   case GFX_H_RIGHT:  fputs(" text-anchor=\"end\"", fp); break;
   case GFX_H_CENTER: fputs(" text-anchor=\"middle\"", fp); break;
   case GFX_H_LEFT: break;
   case GFX_H_NULL: break;
   }
   svg_close_tag_single_line(fp);
   /* support for node->tabwidth missing */
   svg_write_text(fp, node->text);
   svg_end_tag(fp, "text");
   if (node->angle != 0)
     svg_end_tag(fp, "g");
 }
 
 int       gfx_render_svg (gfx_canvas_t *canvas,
                 art_u32 width, art_u32 height,
                 double zoom,
                 gfx_color_t background, FILE *fp){
   gfx_node_t *node = canvas->firstnode;
   fputs(
 "<?xml version=\"1.0\" standalone=\"yes\"?>\n"
 "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\"\n"
 "   \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
 "<!--\n"
 "   SVG file created by RRDtool,\n"
 "   Tobias Oetiker <tobi@oetike.ch>, http://tobi.oetiker.ch\n"
 "\n"
 "   The width/height attributes in the outhermost svg node\n"
 "   are just default sizes for the browser which is used\n"
 "   if the svg file is openened directly without being\n"
 "   embedded in an html file.\n"
 "   The viewBox is the local coord system for rrdtool.\n"
 "-->\n", fp);
   svg_start_tag(fp, "svg");
   fputs(" width=\"", fp);
   svg_write_number(fp, width * zoom);
   fputs("\" height=\"", fp);
   svg_write_number(fp, height * zoom);
   fputs("\" x=\"0\" y=\"0\" viewBox=\"", fp);
   svg_write_number(fp, -LINEOFFSET);
   fputs(" ", fp);
   svg_write_number(fp, -LINEOFFSET);
   fputs(" ", fp);
   svg_write_number(fp, width - LINEOFFSET);
   fputs(" ", fp);
   svg_write_number(fp, height - LINEOFFSET);
   fputs("\" preserveAspectRatio=\"xMidYMid\"", fp);
   fputs(" font-family=\"Helvetica\"", fp); /* default font */
   svg_close_tag(fp);
   svg_start_tag(fp, "rect");
   fprintf(fp, " x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"", width, height);
   fputs(" style=\"fill:", fp);
   svg_write_color(fp, background);
   fputs("\"", fp);
   svg_close_tag_empty_node(fp);
   while (node) {
     switch (node->type) {
     case GFX_LINE:
       svg_multi_path(fp, &node);
       break;
     case GFX_AREA:
       svg_area(fp, node);
       break;
     case GFX_TEXT:
       svg_text(fp, node);
     }
     node = node->next;
   }
   svg_end_tag(fp, "svg");
   return 0;
 }