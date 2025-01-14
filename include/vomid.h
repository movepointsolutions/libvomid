/* (C)opyright 2008 Anton Novikov
 * See LICENSE file for license details.
 */

#ifndef VOMID_H_INCLUDED
#define VOMID_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>  /* FILE */
#include <stdlib.h> /* size_t */
#include <stddef.h> /* offsetof */
#include <limits.h> /* MAX_INT */
#include "stdint_wrap.h" /* uint16_t */

typedef struct vmd_ctrl_info_t vmd_ctrl_info_t;
typedef struct vmd_notesystem_t vmd_notesystem_t;
typedef struct vmd_rendersystem_t vmd_rendersystem_t;
typedef struct vmd_file_t vmd_file_t;
typedef struct vmd_track_t vmd_track_t;
typedef struct vmd_channel_t vmd_channel_t;
typedef struct vmd_note_t vmd_note_t;
typedef struct vmd_bst_t vmd_bst_t;
typedef struct vmd_bst_node_t vmd_bst_node_t;
typedef struct vmd_bst_rev_t vmd_bst_rev_t;
typedef struct vmd_map_t vmd_map_t;
typedef struct vmd_map_bstdata_t vmd_map_bstdata_t;
typedef struct vmd_pool_t vmd_pool_t;
typedef struct vmd_file_rev_t vmd_file_rev_t;
typedef struct vmd_measure_t vmd_measure_t;
typedef struct vmd_play_ctx_t vmd_play_ctx_t;

typedef int vmd_status_t;
#define VMD_ERROR (-1)
#define VMD_OK 0
#define VMD_STOP 1

typedef int vmd_bool_t;
#define VMD_FALSE 0
#define VMD_TRUE 1

#define VMD_DEFINE_DESTROY(type) \
static inline void \
vmd_ ## type ## _destroy(vmd_ ## type ## _t *o) \
{ \
	vmd_ ## type ## _fini(o); \
	free(o); \
}

/* midi stuff */

#define VMD_CHANNELS 16
#define VMD_PROGRAMS 128
#define VMD_MAX_TIME INT_MAX
#define VMD_MAX_PITCH SHRT_MAX

enum {
	VMD_VOICE_NOTEOFF = 0x80,
	VMD_VOICE_NOTEON  = 0x90,
	VMD_VOICE_NOTEAFTERTOUCH = 0xA0,
	VMD_VOICE_CONTROLLER = 0xB0,
	VMD_VOICE_PROGRAM = 0xC0,
	VMD_VOICE_CHANNELPRESSURE = 0xD0,
	VMD_VOICE_PITCHWHEEL = 0xE0,

	VMD_META_TRACKNAME = 0x03,
	VMD_META_EOT = 0x2F,
	VMD_META_PROPRIETARY = 0x7f,

	VMD_CTRL_CONTROLLERS_OFF = 121,
	VMD_CTRL_NOTES_OFF = 123,
};

typedef int vmd_time_t;
typedef short vmd_pitch_t;
typedef signed char vmd_midipitch_t;
typedef signed char vmd_velocity_t;

#define VMD_DRUMCHANNEL 9

typedef uint16_t vmd_chanmask_t;
#define VMD_CHANMASK_ALL ((uint16_t)0xFFFF)
#define VMD_CHANMASK_DRUMS ((uint16_t)(1 << VMD_DRUMCHANNEL))
#define VMD_CHANMASK_NODRUMS (VMD_CHANMASK_ALL & ~VMD_CHANMASK_DRUMS)

#define VMD_TEMPO_MIDI(bpm) (60 * 1000000 / (bpm))
#define VMD_TEMPO_BPM(midi) (60 * 1000000 / (midi))

/* utility: binary logarithm */
#define VMD_BLOG2(n)  ((n) >= 2 ? 1 : 0)
#define VMD_BLOG4(n)  ((n) >= (1 << 2)  ? 2  + VMD_BLOG2(n >> 2)  : VMD_BLOG2(n))
#define VMD_BLOG8(n)  ((n) >= (1 << 4)  ? 4  + VMD_BLOG4(n >> 4)  : VMD_BLOG4(n))
#define VMD_BLOG16(n) ((n) >= (1 << 8)  ? 8  + VMD_BLOG8(n >> 2)  : VMD_BLOG8(n))
#define VMD_BLOG32(n) ((n) >= (1 << 16) ? 16 + VMD_BLOG8(n >> 16) : VMD_BLOG16(n))
#define VMD_BLOG VMD_BLOG32

#define VMD_TIMESIG(numer, denom) ((numer & 0xFF) + (VMD_BLOG(denom) << 8))
#define VMD_TIMESIG_NUMER(timesig) ((timesig) & 0xFF)
#define VMD_TIMESIG_DENOM(timesig) (1 << ((timesig) >> 8))

typedef double vmd_systime_t;

vmd_systime_t vmd_time2systime(vmd_time_t, int, unsigned int);
vmd_time_t vmd_systime2time(vmd_systime_t, int, unsigned int);

enum {
	VMD_FCTRL_TEMPO   = 0x51,
	VMD_FCTRL_TIMESIG = 0x58,

	VMD_MIDI_METAS    = 128,

	VMD_FCTRLS        = VMD_MIDI_METAS
};

enum {
	VMD_CCTRL_VOLUME     = 7,
	VMD_CCTRL_BALANCE    = 8,
	VMD_CCTRL_PAN        = 10,
	VMD_CCTRL_EXPRESSION = 11,

	VMD_MIDI_CTRLS      = 128,

	VMD_CCTRL_PROGRAM    = VMD_MIDI_CTRLS,
	VMD_CCTRL_PITCHWHEEL,
	VMD_CCTRLS
};

extern const char *vmd_gm_program_name[VMD_PROGRAMS];

void vmd_notes_off(void);
void vmd_reset_output(void);

/* bst.c */

struct vmd_bst_node_t {
	vmd_bst_node_t *parent;
	vmd_bst_node_t *child[2];
	vmd_bst_node_t *next; /* for slist of inserted nodes */

	int             balance :3;
	unsigned        idx     :1;
	unsigned        in_tree :1;
	unsigned        inserted:1;
	unsigned        saved   :1;

	/* only used internally in bst_update(),
	 * so we can use it for node marking */
	unsigned was_in_tree    :1;

	char data[0];
};

/*
 * prev(first) = next(last) = head
 * first(empty) = last(empty) = head
 * root(empty) = child(leaf) = NULL
 */

vmd_bst_node_t *vmd_bst_node_child_most(vmd_bst_node_t *node, int dir);
vmd_bst_node_t *vmd_bst_node_adj(vmd_bst_node_t *node, int dir);

#define vmd_bst_node_leftmost(x) vmd_bst_node_child_most(x, 0)
#define vmd_bst_node_rightmost(x) vmd_bst_node_child_most(x, 1)
#define vmd_bst_node_prev(x) vmd_bst_node_adj(x, 0)
#define vmd_bst_node_next(x) vmd_bst_node_adj(x, 1)

#define vmd_bst_begin(t) vmd_bst_node_leftmost(&(t)->head)
#define vmd_bst_end(t) (&(t)->head)
#define vmd_bst_root(t) ((t)->head.child[0])
#define vmd_bst_next(n) vmd_bst_node_next(n)
#define vmd_bst_prev(n) vmd_bst_node_prev(n)

#define vmd_bst_empty(t) ((t)->head.child[0] == NULL)
#define vmd_bst_node_is_end(n) ((n) == (n)->parent)

typedef int  (*vmd_bst_cmp_t)(const void *, const void *);
typedef void (*vmd_bst_upd_t)(vmd_bst_node_t *);

struct vmd_bst_t {
	vmd_bst_node_t  head;
	size_t          dsize;
	size_t          csize;
	vmd_bst_cmp_t   cmp;
	vmd_bst_upd_t   upd;

	vmd_bst_rev_t  *tip;
	vmd_bst_node_t *inserted, *erased, *free, *save;
	size_t          tree_size;
};

void vmd_bst_init(vmd_bst_t *, size_t, size_t, vmd_bst_cmp_t, vmd_bst_upd_t);
void vmd_bst_fini(vmd_bst_t *);

size_t          vmd_bst_size(vmd_bst_t *tree);
void            vmd_bst_clear(vmd_bst_t *);
vmd_bst_node_t *vmd_bst_insert(vmd_bst_t *, const void *);
vmd_bst_node_t *vmd_bst_erase(vmd_bst_t *, vmd_bst_node_t *);
void            vmd_bst_erase_range(vmd_bst_t *, vmd_bst_node_t *, vmd_bst_node_t *);
void            vmd_bst_change(vmd_bst_t *, vmd_bst_node_t *, const void *);

vmd_bst_node_t *vmd_bst_find(vmd_bst_t *tree, const void *data);
vmd_bst_node_t *vmd_bst_bound(vmd_bst_t *tree, const void *data, int bound);
vmd_bst_node_t *vmd_bst_lower_bound(vmd_bst_t *tree, const void *data);
vmd_bst_node_t *vmd_bst_upper_bound(vmd_bst_t *tree, const void *data);

vmd_bst_rev_t  *vmd_bst_commit(vmd_bst_t *);
vmd_bst_node_t *vmd_bst_revert(vmd_bst_t *);
vmd_bst_node_t *vmd_bst_update(vmd_bst_t *, vmd_bst_rev_t *);

#define VMD_BST_FOREACH(node, bst) \
	for (vmd_bst_node_t *_node_ = vmd_bst_begin(bst), *_cont_ = _node_; \
		_node_ != vmd_bst_end(bst); \
		_cont_ = _node_ = vmd_bst_next(_node_)) \
		for (node = _node_; _cont_; _cont_ = NULL)

/* map.c */

struct vmd_map_t {
	vmd_bst_t bst;
	int default_value;
};

struct vmd_map_bstdata_t {
	vmd_time_t time;
	int value;
};

void vmd_map_init(vmd_map_t *, int default_value);
void vmd_map_fini(vmd_map_t *);

int  vmd_map_get(vmd_map_t *, vmd_time_t, vmd_time_t *);
void vmd_map_get_change(vmd_map_t *, vmd_time_t *, int *);
void vmd_map_set(vmd_map_t *, vmd_time_t, int);
void vmd_map_set_range(vmd_map_t *, vmd_time_t, vmd_time_t, int);
void vmd_map_set_node(vmd_map_t *, vmd_bst_node_t *, int);
void vmd_map_copy(vmd_map_t *, vmd_time_t, vmd_time_t, vmd_map_t *, vmd_time_t);
void vmd_map_add(vmd_map_t *, vmd_time_t, vmd_time_t, int);

vmd_bool_t vmd_map_eq(vmd_map_t *, vmd_map_t *, vmd_time_t, vmd_time_t);

vmd_time_t vmd_map_time(vmd_bst_node_t *node);
int        vmd_map_value(vmd_bst_node_t *node);

/* channel.c */

struct vmd_channel_t {
	int number;
	vmd_bst_t notes;
	vmd_map_t ctrl[VMD_CCTRLS];
	vmd_channel_t *next;
};

/* pool.c */

struct vmd_pool_t {
	struct vmd_pool_chunk_t *chunk;
};

void  vmd_pool_init(vmd_pool_t *);
void  vmd_pool_fini(vmd_pool_t *);
void *vmd_pool_alloc(vmd_pool_t *, size_t);
char *vmd_pool_strdup(vmd_pool_t *, const char *);

/* file.c */

#define VMD_MAX_TRACKS 32

struct vmd_file_t {
	unsigned int   division;
	int            tracks;
	vmd_bool_t     force_compatible;

	vmd_track_t   *track[VMD_MAX_TRACKS];
	vmd_channel_t  channel[VMD_CHANNELS];
	vmd_map_t      ctrl[VMD_FCTRLS];
	vmd_track_t   *tracks_list;

	vmd_map_t      measure_index;
	vmd_pool_t     pool;
};

void            vmd_file_init(vmd_file_t *);
void            vmd_file_fini(vmd_file_t *);

char *          vmd_file_copy_string(vmd_file_t *, const char *);
vmd_status_t    vmd_file_flatten(vmd_file_t *);
vmd_file_rev_t *vmd_file_commit(vmd_file_t *);
void            vmd_file_update(vmd_file_t *, vmd_file_rev_t *);

vmd_status_t    vmd_file_import(vmd_file_t *, const char *, vmd_bool_t *sha_ok);
vmd_status_t    vmd_file_export(vmd_file_t *, const char *);

vmd_time_t      vmd_file_length(const vmd_file_t *);
vmd_bool_t      vmd_file_is_compatible(const vmd_file_t *);

struct vmd_measure_t {
	int number;
	int timesig;
	vmd_time_t beg, end;
	vmd_time_t part_size;
};

typedef void (*vmd_measure_clb_t)(const vmd_measure_t *, void *);

//vmd_measure_t * vmd_file_measure(vmd_file_t *, int);
void vmd_file_measures(vmd_file_t *, vmd_time_t, vmd_time_t, vmd_measure_clb_t, void *);
void vmd_file_measure_at(vmd_file_t *, vmd_time_t, vmd_measure_t *);

/* import.c */

vmd_status_t vmd_file_import_f(vmd_file_t *, FILE *, vmd_bool_t *sha_ok);

/* export.c */

vmd_status_t vmd_file_export_f(vmd_file_t *, FILE *);

/* play.c */

typedef void (*vmd_event_clb_t)(unsigned char *, size_t, void *);
typedef vmd_status_t (*vmd_delay_clb_t)(vmd_time_t delay, int tempo, void *);

vmd_status_t vmd_file_play(vmd_file_t *, vmd_time_t, vmd_event_clb_t, vmd_delay_clb_t, void *, vmd_play_ctx_t **);

/* note.c */

struct vmd_note_t {
	vmd_track_t    *track;

	vmd_time_t      on_time, off_time;
	vmd_velocity_t  on_vel,  off_vel;
	vmd_pitch_t     pitch;

	vmd_midipitch_t midipitch;

	//TODO: we could use bst_node_t::was_in_tree
	unsigned char   mark;
	vmd_channel_t  *channel;
	vmd_note_t     *next; // for destructive operations on ranges
};

void vmd_isolate_note(vmd_note_t *);
void vmd_erase_note(vmd_note_t *);
int  vmd_erase_notes(vmd_note_t *);
void vmd_copy_note(vmd_note_t *, vmd_track_t *, vmd_time_t, vmd_pitch_t);
void vmd_note_set_cctrl(vmd_note_t *, int, int);
void vmd_note_set_pitch(vmd_note_t *, vmd_pitch_t);

/* notesystem.c */

struct vmd_notesystem_t {
	int size;
	float *pitches;
	char *scala;
	vmd_pitch_t end_pitch;
};

vmd_notesystem_t vmd_notesystem_import(const char *);
vmd_notesystem_t vmd_notesystem_tet(int);
vmd_notesystem_t vmd_notesystem_midistd(void);
vmd_bool_t       vmd_notesystem_is_midistd(const vmd_notesystem_t *ns);
void             vmd_notesystem_fini(vmd_notesystem_t ns);

vmd_status_t     vmd_pitch_info(const vmd_notesystem_t *ns, vmd_pitch_t pitch,
		vmd_midipitch_t *midipitch, int *wheel);

int              vmd_notesystem_pitch2level(const vmd_notesystem_t *, vmd_pitch_t p);
vmd_pitch_t      vmd_notesystem_level2pitch(const vmd_notesystem_t *, int);
int              vmd_notesystem_levels(const vmd_notesystem_t *);

/* track.c */

struct vmd_track_t {
	vmd_file_t      *file;
	vmd_bst_t        notes;
	vmd_notesystem_t notesystem;

	vmd_chanmask_t   chanmask;
	int              channel_usage[VMD_CHANNELS];
	vmd_channel_t   *temp_channels;
	vmd_track_t     *next;
	int              primary_ctrl_value[VMD_CCTRLS];

	const char      *name;
};

typedef void *(*vmd_note_callback_t)(vmd_note_t *, void *);

void vmd_track_init(vmd_track_t *, vmd_file_t *, vmd_chanmask_t);
void vmd_track_fini(vmd_track_t *);
void vmd_track_clear(vmd_track_t *);

int  vmd_track_get_ctrl(vmd_track_t *track, int ctrl);
void vmd_track_set_ctrl(vmd_track_t *track, int ctrl, int value);

vmd_track_t *vmd_track_create(vmd_file_t *file, vmd_chanmask_t chanmask);
VMD_DEFINE_DESTROY(track) // vmd_track_destroy

void *      vmd_track_for_range(vmd_track_t *, vmd_time_t, vmd_time_t, vmd_note_callback_t, void *);
vmd_note_t *vmd_track_range(vmd_track_t *, vmd_time_t, vmd_time_t, vmd_pitch_t, vmd_pitch_t);
vmd_note_t *vmd_track_insert(vmd_track_t *, vmd_time_t, vmd_time_t, vmd_pitch_t);

vmd_note_t *vmd_track_note(vmd_bst_node_t *node);

vmd_bool_t      vmd_track_is_drums(const vmd_track_t *);
vmd_time_t      vmd_track_length(const vmd_track_t *);
int             vmd_track_idx(const vmd_track_t *);
void            vmd_track_set_notesystem(vmd_track_t *, vmd_notesystem_t);

/* hal.c */

vmd_systime_t   vmd_systime(void);
void            vmd_sleep(vmd_systime_t);
void            vmd_sleep_till(vmd_systime_t);

enum {
	VMD_INPUT_DEVICE,
	VMD_OUTPUT_DEVICE,
	VMD_DEVICE_TYPES
};

typedef void (*vmd_device_clb_t)(const char *, const char *, void *);

void            vmd_enum_devices(int type, vmd_device_clb_t, void *);
vmd_status_t    vmd_set_device(int type, const char *);

//              vmd_input
void            vmd_output(const unsigned char *, size_t);
void            vmd_flush_output(void);

#ifdef __cplusplus
} // extern "C"
#endif
#define VMD_VOICE_NOTEOFF 0x80
#define VMD_VOICE_NOTEON  0x90
#define VMD_VOICE_AFTERTOUCH 0xA0
#define VMD_VOICE_CONTROLLER 0xB0
#define VMD_VOICE_PROGRAM 0xC0
#define VMD_VOICE_CHANPRESSURE 0xD0

#endif /* VOMID_H_INCLUDED */
