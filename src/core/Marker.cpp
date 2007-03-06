/*
Copyright (C) 2007 Remon Sijrier 

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

*/

#include "Marker.h"

#include "TimeLine.h"

Marker::Marker(TimeLine* tl, nframes_t when)
	: ContextItem()
	, m_timeline(tl)
	, m_when(when) 
{
	set_history_stack(m_timeline->get_history_stack());
}

QDomNode Marker::get_state(QDomDocument doc)
{
	QDomElement domNode = doc.createElement("Marker");
	
	// TODO add all attributes that have to be saved to the dom element
	
	return domNode;
}

int Marker::set_state(const QDomNode & node)
{
	QDomElement e = node.toElement();
	
	// TODO retreive all the attributes to be restored from the dom element
	
	return 1;
}

void Marker::set_when(nframes_t when)
{
	m_when = when;
	emit positionChanged();
}

void Marker::set_description(const QString & des)
{
	m_description = des;
}

//eof