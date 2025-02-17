/* (C)opyright 2008 Anton Novikov
 * See LICENSE file for license details.
 *
 * vomid_local.h
 */

#ifndef VOMID_LOCAL_H_INCLUDED
#define VOMID_LOCAL_H_INCLUDED

#include "vomid.h"
#include "vomid_shortnames.h"

typedef unsigned char uchar;
typedef struct vmd_channel_rev_t vmd_channel_rev_t;
typedef struct vmd_track_note_t vmd_track_note_t;
typedef struct vmd_track_rev_t vmd_track_rev_t;
typedef struct vmd_platform_t vmd_platform_t;
typedef struct vmd_small_event_t vmd_small_event_t;

#define VMD_SHA1_SIZE 20

/* midi stuff */

#define VMD_NOTES 128
#define VMD_METAS 128
#define VMD_CTRLS 128

#define VMD_DEFAULT_VELOCITY 64

enum {
	VMD_PROPR_NOTESYSTEM,
	VMD_PROPR_PITCH,
	VMD_PROPR_SHA
};

/* midi.c */

struct vmd_ctrl_info_t {
	const char  *name;
	int          default_value;
	int  (*read)(unsigned char *buf, int size); /* used only for metas */
	void (*write)(vmd_small_event_t *ev, int channel, int type, int value);
};

extern vmd_ctrl_info_t vmd_fctrl_info[VMD_FCTRLS];
extern vmd_ctrl_info_t vmd_cctrl_info[VMD_CCTRLS];

extern const uchar vmd_magic_mthd[4];
extern const uchar vmd_magic_mtrk[4];
extern const uchar vmd_magic_vomid[4];

#define MAX_SMALL_EVENT_LENGTH 32

struct vmd_small_event_t
{
	uchar buf[MAX_SMALL_EVENT_LENGTH];
	int len;
};

void vmd_midi_write_noteon(small_event_t *ev, note_t *note);
void vmd_midi_write_noteoff(small_event_t *ev, note_t *note);
void vmd_midi_write_meta(small_event_t *ev, uchar type, const uchar *data, int len);

void vmd_midi_fwrite_varlen(FILE *out, time_t time);
void vmd_midi_fwrite_meta_header(FILE *out, uchar type, int len);
void vmd_midi_fwrite_meta(FILE *out, uchar type, const uchar *data, int len);
void vmd_midi_fwrite_propr(FILE *out, uchar type, uchar *data, int s);
void vmd_midi_fwrite_notesystem(FILE *out, const notesystem_t *);
void vmd_midi_fwrite_pitch(FILE *out, pitch_t);

extern const uchar vmd_midi_eot[3];

#define META_HEADER_SIZE 3
#define PROPR_HEADER_SIZE (sizeof(vmd_magic_vomid) + 1)

/* preprocessor utils */

#define VMD_JOIN(x, y) VMD_DO_JOIN(x, y)
#define VMD_DO_JOIN(x, y) VMD_DO_JOIN2(x, y)
#define VMD_DO_JOIN2(x, y) x##y

#define VMD_JOIN3(x, y, z) VMD_JOIN(VMD_JOIN(x, y), z)

#define VMD_STRINGIFY(x) VMD_DO_STRINGIFY(x)
#define VMD_DO_STRINGIFY(x) #x

#define VMD_LENGTH(x) (sizeof(x) / sizeof(*x))

#define VMD_MIN(x, y) ((x) < (y) ? (x) : (y))
#define VMD_MAX(x, y) ((x) < (y) ? (y) : (x))
#define VMD_MIN3(x, y, z) VMD_MIN(VMD_MIN(x, y), z)
#define VMD_MAX3(x, y, z) VMD_MAX(VMD_MAX(x, y), z)

#define VMD_SWAP(x, y, type) do { \
	type tmp; \
	tmp = x; \
	x = y; \
	y = tmp; \
} while(0)

/* comparator functions helper */
#define VMD_CMP(a, b) do { \
	if (a != b) \
		return a - b; \
} while(0)

/* stack.c */

typedef struct vmd_stack_t {
	struct vmd_stack_block_t *head;
	size_t dsize;
} vmd_stack_t;

void            vmd_stack_init(vmd_stack_t *, size_t dsize);
void            vmd_stack_fini(vmd_stack_t *);
void *          vmd_stack_push(vmd_stack_t *, const void *);

/*
 * the pointer returned is valid until next call to
 * any set-modifying function (destroy, push, pop)
 */
void *          vmd_stack_pop(vmd_stack_t *);

/* note.c */

vmd_note_t *    vmd_insert_note(const vmd_note_t *note);
void            vmd_note_set_channel(vmd_note_t *note, vmd_channel_t *);
void            vmd_note_reset_pitch(vmd_note_t *note, vmd_pitch_t);

int             vmd_note_cmp(const vmd_note_t *, const vmd_note_t *);

/* notesystem.c */

vmd_notesystem_t vmd_notesystem_import_f(FILE *);

/* channel.c */

struct vmd_channel_rev_t {
	vmd_bst_rev_t *notes;
	vmd_bst_rev_t *ctrl[VMD_CCTRLS];
};

void        vmd_channel_init(vmd_channel_t *, int number);
void        vmd_channel_fini(vmd_channel_t *);

vmd_channel_t *vmd_channel_create(int number);
VMD_DEFINE_DESTROY(channel) // vmd_channel_destroy

void *      vmd_channel_range(vmd_channel_t *, vmd_time_t, vmd_time_t, vmd_note_callback_t, void *);

void        vmd_channel_commit(vmd_channel_t *, vmd_channel_rev_t *);
void        vmd_channel_update(vmd_channel_t *, vmd_channel_rev_t *);

vmd_note_t *vmd_channel_note(vmd_bst_node_t *node);

/* track.c */

struct vmd_track_note_t {
	vmd_note_t note;
	vmd_time_t max_off;
};

struct vmd_track_rev_t {
	vmd_bst_rev_t *notes;
	const char *name;
	int primary_ctrl_value[VMD_CCTRLS];
};

vmd_status_t        vmd_track_flatten(vmd_track_t *);
vmd_channel_t *     vmd_track_temp_channel(vmd_track_t *, vmd_time_t, vmd_time_t, vmd_note_t *);

void                vmd_track_commit(vmd_track_t *, vmd_track_rev_t *);
void                vmd_track_update(vmd_track_t *, vmd_track_rev_t *);

/* file.c */

struct vmd_file_rev_t {
	int tracks;
	vmd_track_t *track[VMD_MAX_TRACKS];
	vmd_track_rev_t track_rev[VMD_MAX_TRACKS];
	vmd_channel_rev_t channel[VMD_CHANNELS];
	vmd_bst_rev_t *ctrl[VMD_FCTRLS];
};

/* play.c */

typedef void         (*vmd_tevent_clb_t)(int track, vmd_small_event_t *ev, void *arg);
typedef vmd_status_t (*vmd_dtime_clb_t)(vmd_time_t delay, void *arg);
typedef void         (*vmd_note_clb_t)(const note_t *note, void *arg);

vmd_status_t         vmd_file_play_(vmd_file_t *file, vmd_time_t time, vmd_tevent_clb_t voice_clb,
						vmd_dtime_clb_t dtime_clb, vmd_note_clb_t note_clb, void *arg, vmd_play_ctx_t **pctx);

/* bst.c */

struct vmd_bst_rev_t {
	vmd_bst_rev_t *parent;
	vmd_bst_rev_t *brother;
	vmd_bst_rev_t *child;

	int inserted_count, erased_count, changed_count;
	vmd_bst_node_t **inserted, **erased, **changed;
	void *uninserted_data, *erased_data, *changed_data;
};

static inline vmd_bst_node_t *
vmd_bst_node(void *data)
{
	return data - offsetof(vmd_bst_node_t, data);
}

/* range functions; need on(), off(), max_off() and note() to be defined */

#define VMD_DEFINE_RANGE_FN \
static void * \
range_(vmd_bst_node_t *node, vmd_time_t s, vmd_time_t e, vmd_note_callback_t clb, void *arg) \
{ \
	vmd_bst_node_t *l = node->child[0], *r = node->child[1]; \
	void *res; \
	if (s < off(node) && on(node) < e) { \
		if ((res = clb(note(node), arg)) != NULL) \
			return res; \
	} \
	if (l != NULL && max_off(l) > s) { \
		if ((res = range_(l, s, e, clb, arg)) != NULL) \
			return res; \
	} \
	if (r != NULL && max_off(r) > s && on(node) < e) { \
		if ((res = range_(r, s, e, clb, arg)) != NULL) \
			return res; \
	} \
	return NULL; \
} \
 \
static void * \
range(vmd_bst_t *bst, vmd_time_t s, vmd_time_t e, vmd_note_callback_t clb, void *arg) \
{ \
	vmd_bst_node_t *root = vmd_bst_root(bst); \
	return root == NULL ? NULL : range_(root, s, e, clb, arg); \
} \

/* hal.c */

struct vmd_platform_t {
	vmd_status_t (*init)();
	void (*fini)();
	void (*enum_devices)(int type, vmd_device_clb_t, void *);
	vmd_status_t (*set_device)(int, const char *);
	void (*output)(const uchar *, size_t);
	void (*flush_output)();
	const char *name;
	vmd_bool_t initialized;

	vmd_platform_t *next;
};

extern vmd_platform_t vmd_platform_alsa;
extern vmd_platform_t vmd_platform_win32;

#endif /* VOMID_LOCAL_H_INCLUDED */
