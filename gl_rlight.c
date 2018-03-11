/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "gl_model.h"
#include "gl_local.h"
#include "gl_billboards.h"

int	r_dlightframecount;


void R_AnimateLight (void) {
	int i, j, l1, l2, i1, i2;
	float lerpfrac;

	// light animations : 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int) (r_refdef2.time * 10);
	lerpfrac = r_refdef2.time * 10 - i;
	for (j = 0; j < MAX_LIGHTSTYLES; j++) {
		if (!cl_lightstyle[j].length) {
			d_lightstylevalue[j] = 256;
			continue;
		}

		i1 = l1 = i % cl_lightstyle[j].length;
		l1 = (cl_lightstyle[j].map[l1] - 'a') * 22;
		i2 = l2 = (i + 1) % cl_lightstyle[j].length;
		l2 = (cl_lightstyle[j].map[l2] - 'a') * 22;

		if (l1 - l2 > 220 || l2 - l1 > 220) {
			d_lightstylevalue[j] = l2;
		}
		else {
			d_lightstylevalue[j] = l1 + (l2 - l1) * lerpfrac;
		}
	}
}


float bubble_sintable[17], bubble_costable[17];

void R_InitBubble(void) {
	float a, *bub_sin, *bub_cos;
	int i;

	bub_sin = bubble_sintable;
	bub_cos = bubble_costable;

	for (i = 16; i >= 0; i--) {
		a = i/16.0 * M_PI*2;
		*bub_sin++ = sin(a);
		*bub_cos++ = cos(a);
	}
}

//VULT LIGHTS
/*float bubblecolor[NUM_DLIGHTTYPES][4] = {
	{ 0.2, 0.1, 0.05 },		// dimlight or brightlight (lt_default)
	{ 0.2, 0.1, 0.05 },		// muzzleflash
	{ 0.2, 0.1, 0.05 },		// explosion
	{ 0.2, 0.1, 0.05 },		// rocket
	{ 0.5, 0.05, 0.05 },	// red
	{ 0.05, 0.05, 0.3 },	// blue
	{ 0.5, 0.05, 0.4 },		// red + blue
	{ 0.05, 0.45, 0.05 },	// green
	{ 0.5, 0.5, 0.5},		// white
	{ 0.5, 0.5, 0.5},		// custom
};*/
//VULT LIGHTS - My lighting colours are different, I don't know why, but they've been this way since 0.31 or so
float bubblecolor[NUM_DLIGHTTYPES][4] = {
	{ 0.4,  0.2,  0.1  },	// dimlight or brightlight (lt_default)
	{   1,  0.4,  0.2  },	// muzzleflash
	{ 0.8,  0.4,  0.2  },	// explosion
	{ 0.2,  0.1,  0.05 },	// rocket  fuh : lightbubble
	{ 0.5,  0.05, 0.05 },	// red
	{ 0.05, 0.05, 0.3  },	// blue
	{ 0.5,  0.05, 0.4  },	// red + blue
	{ 0.05, 0.45, 0.05 },	// green
	{ 0.5,  0.45, 0.05 },	// red + green
	{ 0.05, 0.45, 0.4  },	// blue + green
	{ 0.5,  0.5,  0.5  },	// white
	{ 0.5,  0.5,  0.5  },	// custom
};

static qbool first_dlight;

void R_RenderDlight(dlight_t *light)
{
	// don't draw our own powerup glow and muzzleflashes
	// muzzleflash keys are negative
	if (light->key == (cl.viewplayernum + 1) || light->key == -(cl.viewplayernum + 1)) {
		return;
	}

	if (first_dlight) {
		GL_BillboardInitialiseBatch(BILLBOARD_FLASHBLEND_LIGHTS, GL_ONE, GL_ONE_MINUS_SRC_ALPHA, null_texture_reference, 0, GL_TRIANGLE_FAN, true, false);

		first_dlight = false;
	}

	{
		int i, j;
		vec3_t v, v_right, v_up;
		float length;
		float rad = light->radius * 0.35;
		float *bub_sin, *bub_cos;
		GLubyte center_color[4] = { 255, 255, 255, 255 };
		GLubyte outer_color[4] = { 0, 0, 0, 0 };
		float alpha = 0.7f;

		VectorSubtract(light->origin, r_origin, v);
		length = VectorNormalize(v);

		if (length < rad) {
			// view is inside the dlight
			V_AddLightBlend(1, 0.5, 0, light->radius * 0.0003);
			return;
		}

		if (!GL_BillboardAddEntry(BILLBOARD_FLASHBLEND_LIGHTS, 18)) {
			return;
		}

		VectorVectors(v, v_right, v_up);

		if (length - rad > 8) {
			VectorScale(v, rad, v);
		}
		else {
			// make sure the light bubble will not be clipped by near z clip plane
			VectorScale(v, length - 8, v);
		}

		VectorSubtract(light->origin, v, v);

		if (light->type == lt_custom) {
			memcpy(center_color, light->color, 3);
		}
		else {
			// FIXME: bubblecolor has 4 elements, but inline spec is only 3... ?
			center_color[0] = bubblecolor[light->type][0] * 255;
			center_color[1] = bubblecolor[light->type][1] * 255;
			center_color[2] = bubblecolor[light->type][2] * 255;
		}

		center_color[0] *= alpha;
		center_color[1] *= alpha;
		center_color[2] *= alpha;
		center_color[3] = 255 * alpha;

		GL_BillboardAddVert(BILLBOARD_FLASHBLEND_LIGHTS, v[0], v[1], v[2], 1, 1, center_color);

		bub_sin = bubble_sintable;
		bub_cos = bubble_costable;

		for (i = 16; i >= 0; i--) {
			for (j = 0; j < 3; j++) {
				v[j] = light->origin[j] + (v_right[j] * (*bub_cos) + +v_up[j] * (*bub_sin)) * rad;
			}
			bub_sin++;
			bub_cos++;

			GL_BillboardAddVert(BILLBOARD_FLASHBLEND_LIGHTS, v[0], v[1], v[2], 1, 1, outer_color);
		}
	}
}

void R_RenderDlights(void)
{
	unsigned int i;
	unsigned int j;
	dlight_t *l;

	r_dlightframecount = r_framecount;
	first_dlight = true;

	for (i = 0; i < MAX_DLIGHTS / 32; i++) {
		if (cl_dlight_active[i]) {
			for (j = 0; j < 32; j++) {
				if ((cl_dlight_active[i] & (1 << j)) && i * 32 + j < MAX_DLIGHTS) {
					l = cl_dlights + i * 32 + j;

					if (l->bubble == 2) { // light from rl
						if (gl_rl_globe.integer & 1) {
							R_RenderDlight(l);
						}
						if (gl_rl_globe.integer & 2) {
							R_MarkLights(l, 1 << (i * 32 + j), cl.worldmodel->nodes);
						}
						continue;
					}

					R_MarkLights(l, 1 << (i * 32 + j), cl.worldmodel->nodes);

					if (gl_flashblend.integer && !(l->bubble && gl_flashblend.integer != 2)) {
						R_RenderDlight(l);
					}
				}
			}
		}
	}
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
//VULT: Lord Havoc light stuff
void R_MarkLights(dlight_t *light, int bit, mnode_t *node)
{
	float		dist;
	msurface_t	*surf;
	int			i;

	if (r_dynamic.integer != 1) {
		return;
	}

loc0:
	if (node->contents < 0) {
		return;
	}

	dist = PlaneDiff(light->origin, node->plane);
	if (dist > light->radius) {
		// LordHavoc: .lit support begin (actually this is just a major lighting speedup, no relation to color :)
		node = node->children[0];
		goto loc0;
		// LordHavoc: .lit support end
	}
	if (dist < -light->radius) {
		// LordHavoc: .lit support begin (actually this is just a major lighting speedup, no relation to color :)
		node = node->children[1];
		goto loc0;
		// LordHavoc: .lit support end
	}

	// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++) {
		// LordHavoc: .lit support begin (actually this is just a major lighting speedup, no relation to color :)
		if (surf->dlightframe != r_dlightframecount) {
			// not dynamic until now
			surf->dlightbits = bit;
			surf->dlightframe = r_dlightframecount;
		}
		else {
			// already dynamic
			surf->dlightbits |= bit;
		}
	}
	// LordHavoc: .lit support begin (actually this is just a major lighting speedup, no relation to color :)
	if (node->children[0]->contents >= 0) {
		R_MarkLights(light, bit, node->children[0]); // LordHavoc: original code
	}
	if (node->children[1]->contents >= 0) {
		R_MarkLights(light, bit, node->children[1]); // LordHavoc: original code
	}
	// LordHavoc: .lit support end
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t		*lightplane;
vec3_t			lightspot;
// LordHavoc: .lit support begin
// LordHavoc: original code replaced entirely
int RecursiveLightPoint (vec3_t color, mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	vec3_t		mid;

loc0:
	if (node->contents < 0)
		return false;		// didn't hit anything
	
// calculate mid point
	if (node->plane->type < 3)
	{
		front = start[node->plane->type] - node->plane->dist;
		back = end[node->plane->type] - node->plane->dist;
	}
	else
	{
		front = DotProduct(start, node->plane->normal) - node->plane->dist;
		back = DotProduct(end, node->plane->normal) - node->plane->dist;
	}

	// LordHavoc: optimized recursion
	if ((back < 0) == (front < 0))
//		return RecursiveLightPoint (color, node->children[front < 0], start, end);
	{
		node = node->children[front < 0];
		goto loc0;
	}
	
	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	
// go down front side
	if (RecursiveLightPoint (color, node->children[front < 0], start, mid))
		return true;	// hit something
	else
	{
		int i, ds, dt;
		msurface_t *surf;
	// check for impact on this node
		VectorCopy (mid, lightspot);
		lightplane = node->plane;

		surf = cl.worldmodel->surfaces + node->firstsurface;
		for (i = 0;i < node->numsurfaces;i++, surf++)
		{
			if (surf->flags & SURF_DRAWTILED)
				continue;	// no lightmaps

			ds = (int) ((float) DotProduct (mid, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3]);
			dt = (int) ((float) DotProduct (mid, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3]);

			if (ds < surf->texturemins[0] || dt < surf->texturemins[1])
				continue;
			
			ds -= surf->texturemins[0];
			dt -= surf->texturemins[1];
			
			if (ds > surf->extents[0] || dt > surf->extents[1])
				continue;

			if (surf->samples)
			{
				// LordHavoc: enhanced to interpolate lighting
				byte *lightmap;
				int maps, line3, dsfrac = ds & 15, dtfrac = dt & 15, r00 = 0, g00 = 0, b00 = 0, r01 = 0, g01 = 0, b01 = 0, r10 = 0, g10 = 0, b10 = 0, r11 = 0, g11 = 0, b11 = 0;
				float scale;
				line3 = ((surf->extents[0]>>4)+1)*3;

				lightmap = surf->samples + ((dt>>4) * ((surf->extents[0]>>4)+1) + (ds>>4))*3; // LordHavoc: *3 for color

				for (maps = 0;maps < MAXLIGHTMAPS && surf->styles[maps] != 255;maps++)
				{
					scale = (float) d_lightstylevalue[surf->styles[maps]] * 1.0 / 256.0;
					r00 += (float) lightmap[      0] * scale;g00 += (float) lightmap[      1] * scale;b00 += (float) lightmap[2] * scale;
					r01 += (float) lightmap[      3] * scale;g01 += (float) lightmap[      4] * scale;b01 += (float) lightmap[5] * scale;
					r10 += (float) lightmap[line3+0] * scale;g10 += (float) lightmap[line3+1] * scale;b10 += (float) lightmap[line3+2] * scale;
					r11 += (float) lightmap[line3+3] * scale;g11 += (float) lightmap[line3+4] * scale;b11 += (float) lightmap[line3+5] * scale;
					lightmap += ((surf->extents[0]>>4)+1) * ((surf->extents[1]>>4)+1)*3; // LordHavoc: *3 for colored lighting
				}

				color[0] += (float) ((int) ((((((((r11-r10) * dsfrac) >> 4) + r10)-((((r01-r00) * dsfrac) >> 4) + r00)) * dtfrac) >> 4) + ((((r01-r00) * dsfrac) >> 4) + r00)));
				color[1] += (float) ((int) ((((((((g11-g10) * dsfrac) >> 4) + g10)-((((g01-g00) * dsfrac) >> 4) + g00)) * dtfrac) >> 4) + ((((g01-g00) * dsfrac) >> 4) + g00)));
				color[2] += (float) ((int) ((((((((b11-b10) * dsfrac) >> 4) + b10)-((((b01-b00) * dsfrac) >> 4) + b00)) * dtfrac) >> 4) + ((((b01-b00) * dsfrac) >> 4) + b00)));
			}
			return true; // success
		}

	// go down back side
		return RecursiveLightPoint (color, node->children[front >= 0], mid, end);
	}
}

vec3_t lightcolor; // LordHavoc: used by model rendering
int R_LightPoint (vec3_t p)
{
	vec3_t		end;
	qbool		full_light;
	
	full_light = (R_FullBrightAllowed() || !cl.worldmodel->lightdata);
	if (full_light && !r_shadows.value)
		goto skip_trace;  // go grab yourself a copy of "'GOTO Considered Harmful' Considered Harmful"

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 8192;

	lightcolor[0] = lightcolor[1] = lightcolor[2] = 0;
	RecursiveLightPoint (lightcolor, cl.worldmodel->nodes, p, end);
	if (full_light) {
skip_trace:
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 255;
		return 255;
	}
	return ((lightcolor[0] + lightcolor[1] + lightcolor[2]) * (1.0f / 3.0f));
}
// LordHavoc: .lit support end
