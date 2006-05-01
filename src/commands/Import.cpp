/*
Copyright (C) 2005-2006 Remon Sijrier 

This file is part of Traverso

Traverso is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.

$Id: Import.cpp,v 1.2 2006/05/01 21:15:40 r_sijrier Exp $
*/

#include <libtraversocore.h>

#include <QFileDialog>
#include <ReadSource.h>

#include "Import.h"

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"


Import::Import(Track* track)
		: Command(track)
{
	m_track = track;
}


Import::Import(Track* track, QString fileName)
		: Command(track)
{
	m_track = track;
	m_fileName = fileName;
}

Import::~Import()
{}

int Import::prepare_actions()
{
	PENTER;
	if (m_fileName.isEmpty()) {
		m_fileName = QFileDialog::getOpenFileName(0,
				tr("Import audio source"),
				getenv("HOME"),
				tr("All files (*);;Audio files (*.wav *.flac)"));
	}

	if (m_fileName.isEmpty()) {
		PWARN("FileName is empty!");
		return 0;
	}

	int splitpoint = m_fileName.lastIndexOf("/") + 1;
	int length = m_fileName.length();

	QString dir = m_fileName.left(splitpoint - 1);
	QString name = m_fileName.right(length - splitpoint);

	Project* project = pm().get_project();
	if (!project) {
		PWARN("No project loaded, can't import an AudioSource without a Project");
		return 0;
	}

	ReadSource* source = new ReadSource(0, dir, name);

	if (source->init() < 0) {
		PWARN("AudioSource init failed");
		delete source;
		return 0;
	}


	m_clip = new AudioClip(m_track, 0, name);

	int channels = source->get_channel_count();
	ReadSource* existingSource;

	for (int channel=0; channel < channels; channel++) {
		
		if ( (existingSource = project->get_audiosource_manager()->get_source( m_fileName, channel)) != 0) {
			PWARN("Using existing AudioSource object");
			m_clip->add_audio_source(existingSource, channel);
		} else {
			PWARN("Creating new AudioSource object");
			ReadSource* newSource = project->get_audiosource_manager()->new_readsource(dir, name, channel, 0, 0);

			if (newSource->init() < 0) {
				// Hmmmm this will mess up certain things a little but is unlikey to happen....
				PERROR("Failed to initialize ReadSource %s for channel %d", m_fileName.toAscii().data(), channel);
				return -1;
			}

			m_clip->add_audio_source(newSource, channel);
		}
	}

	delete source;

	return 1;
}

int Import::do_action()
{
	PENTER;
	m_track->add_clip(m_clip);
	return 1;
}


int Import::undo_action()
{
	PENTER;
	m_track->remove_clip(m_clip);
	return 1;
}


// eof


