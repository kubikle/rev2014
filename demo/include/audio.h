#pragma once
#ifndef AUDIO_H
#define AUDIO_H

typedef struct {
	const struct sync_track *cam, *camTime,*camFov;
	const struct sync_track *camRotX, *camRotY, *camRotZ;
	const struct sync_track *saturation, *fade;
	const struct sync_track *synk1, *synk2, *synk3;
	const struct sync_track *synk4, *synk5, *synk6;
	const struct sync_track *synk7, *synk8, *synk9;
	const struct sync_track *scene;
	const struct sync_track *blur;
	const struct sync_track *blurPower;
	const struct sync_track *bloom;
	const struct sync_track *bloomPower;
	const struct sync_track *radial;
	const struct sync_track *noise;
	const struct sync_track *image;
	const struct sync_track *imageFade;
} SyncTracks; 

#endif