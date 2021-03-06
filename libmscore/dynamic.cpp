//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "dynamic.h"
#include "xml.h"
#include "score.h"
#include "measure.h"
#include "system.h"
#include "segment.h"
#include "utils.h"
#include "style.h"
#include "mscore.h"

namespace Ms {

//-----------------------------------------------------------------------------
//   Dyn
//    see: http://en.wikipedia.org/wiki/File:Dynamic's_Note_Velocity.svg
//-----------------------------------------------------------------------------

struct Dyn {
      int velocity;      ///< associated midi velocity (0-127, -1 = none)
      bool accent;       ///< if true add velocity to current chord velocity
      const char* tag;   // name of dynamics, eg. "fff"
      const char* text;  // utf8 text of dynamic
      };

static Dyn dynList[] = {
      // dynamic:
      {  -1,  true,  "other-dynamics", ""                                                       },
      {   1,  false, "pppppp", u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f" },
      {   5,  false, "ppppp",  u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f"           },
      {  10,  false, "pppp",   u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f"                     },
      {  16,  false, "ppp",    u8"\U0001d18f\U0001d18f\U0001d18f"                               },
      {  33,  false, "pp",     u8"\U0001d18f\U0001d18f"                                         },
      {  49,  false, "p",      u8"\U0001d18f"                                                   },
      {  64,  false, "mp",     u8"\U0001d190\U0001d18f"                                         },
      {  80,  false, "mf",     u8"\U0001d190\U0001d191"                                         },
      {  96,  false, "f",      u8"\U0001d191"                                                   },
      { 112,  false, "ff",     u8"\U0001d191\U0001d191"                                         },
      { 126,  false, "fff",    u8"\U0001d191\U0001d191\U0001d191"                               },
      { 127,  false, "ffff",   u8"\U0001d191\U0001d191\U0001d191\U0001d191"                     },
      { 127,  false, "fffff",  u8"\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191"           },
      { 127,  false, "ffffff", u8"\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191" },

      // accents:
      {  0,   true,  "fp",     u8"\U0001d191\U0001d18f"},
      {  0,   true,  "sf",     u8"\U0001d18d\U0001d191"},
      {  0,   true,  "sfz",    u8"\U0001d18d\U0001d191\U0001d18e"},
      {  0,   true,  "sff",    u8"\U0001d18d\U0001d191\U0001d191"},
      {  0,   true,  "sffz",   u8"\U0001d18d\U0001d191\U0001d191\U0001d18e"},
      {  0,   true,  "sfp",    u8"\U0001d18d\U0001d191\U0001d18f"},
      {  0,   true,  "sfpp",   u8"\U0001d18d\U0001d191\U0001d18f\U0001d18f"},
      {  0,   true,  "rfz",    u8"\U0001d18c\U0001d191\U0001d18e"},
      {  0,   true,  "rf",     u8"\U0001d18c\U0001d191"},
      {  0,   true,  "fz",     u8"\U0001d191\U0001d18e"},
      {  0,   true,  "m",      u8"\U0001d190"},
      {  0,   true,  "r",      u8"\U0001d18c"},
      {  0,   true,  "s",      u8"\U0001d18d"},
      {  0,   true,  "z",      u8"\U0001d18e"},
      };

//---------------------------------------------------------
//   Dynamic
//---------------------------------------------------------

Dynamic::Dynamic(Score* s)
   : Text(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      _velocity = -1;
      _dynRange = DYNAMIC_PART;
      setTextStyleType(TEXT_STYLE_DYNAMICS);
      _dynamicType  = DYNAMIC_OTHER;
      }

Dynamic::Dynamic(const Dynamic& d)
   : Text(d)
      {
      _dynamicType   = d._dynamicType;
      _velocity  = d._velocity;
      _dynRange  = d._dynRange;
      }

//---------------------------------------------------------
//   setVelocity
//---------------------------------------------------------

void Dynamic::setVelocity(int v)
      {
      _velocity = v;
      }

//---------------------------------------------------------
//   velocity
//---------------------------------------------------------

int Dynamic::velocity() const
      {
      return _velocity <= 0 ? dynList[dynamicType()].velocity : _velocity;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Dynamic::write(Xml& xml) const
      {
      xml.stag("Dynamic");
      xml.tag("subtype", dynamicTypeName());
      writeProperty(xml, P_VELOCITY);
      writeProperty(xml, P_DYNAMIC_RANGE);
      Text::writeProperties(xml, dynamicType() == 0);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Dynamic::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag = e.name();
            if (tag == "subtype") {
                  setDynamicType(e.readElementText());
                  }
            else if (tag == "velocity")
                  _velocity = e.readInt();
            else if (tag == "dynType")
                  _dynRange = DynamicRange(e.readInt());
            else if (!Text::readProperties(e))
                  e.unknown();
            }
      setTextStyleType(TEXT_STYLE_DYNAMICS);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Dynamic::layout()
      {
      if (!readPos().isNull()) {
            if (score()->mscVersion() < 118) {
                  setReadPos(QPointF());
                  // hack: 1.2 boundingBoxes are a bit wider which results
                  // in symbols moved right
                  setUserXoffset(userOff().x() - spatium() * .6);
                  }
            }
      Text::layout();
      }

//---------------------------------------------------------
//   setDynamicType
//---------------------------------------------------------

void Dynamic::setDynamicType(const QString& tag)
      {
      int n = sizeof(dynList)/sizeof(*dynList);
      for (int i = 0; i < n; ++i) {
            if (dynList[i].tag == tag) {
                  setDynamicType(DynamicType(i));
                  setText(QString::fromUtf8(dynList[i].text));
                  return;
                  }
            }
      setDynamicType(DYNAMIC_OTHER);
      setText(tag);
      }

//---------------------------------------------------------
//   dynamicTypeName
//---------------------------------------------------------

QString Dynamic::dynamicTypeName() const
      {
      return dynList[dynamicType()].tag;
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Dynamic::startEdit(MuseScoreView* v, const QPointF& p)
      {
      Text::startEdit(v, p);
      }

void Dynamic::endEdit()
      {
      Text::endEdit();
      if (!styled() || text() != QString::fromUtf8(dynList[_dynamicType].text))
            _dynamicType = DYNAMIC_OTHER;
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Dynamic::reset()
      {
//      setDynamicType(getText());
      Text::reset();
      }

//---------------------------------------------------------
//   dragAnchor
//---------------------------------------------------------

QLineF Dynamic::dragAnchor() const
      {
      qreal xp = 0.0;
      for (Element* e = parent(); e; e = e->parent())
            xp += e->x();
      qreal yp = measure()->system()->staffY(staffIdx());
      QPointF p(xp, yp);
      return QLineF(p, canvasPos());
      }

//---------------------------------------------------------
//   undoSetDynRange
//---------------------------------------------------------

void Dynamic::undoSetDynRange(DynamicRange v)
      {
      score()->undoChangeProperty(this, P_DYNAMIC_RANGE, v);
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Dynamic::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            case P_DYNAMIC_RANGE:     return int(_dynRange);
            case P_VELOCITY:          return _velocity;
            case P_SUBTYPE:           return _dynamicType;
            default:
                  return Text::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Dynamic::setProperty(P_ID propertyId, const QVariant& v)
      {
      switch (propertyId) {
            case P_DYNAMIC_RANGE:
                  _dynRange = DynamicRange(v.toInt());
                  break;
            case P_VELOCITY:
                  _velocity = v.toInt();
                  break;
            case P_SUBTYPE:
                  _dynamicType = DynamicType(v.toInt());
                  break;
            default:
                  if (!Text::setProperty(propertyId, v))
                        return false;
                  break;
            }
      score()->setLayoutAll(true);
      return true;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Dynamic::propertyDefault(P_ID id) const
      {
      switch(id) {
            case P_TEXT_STYLE:    return TEXT_STYLE_DYNAMICS;
            case P_DYNAMIC_RANGE: return DYNAMIC_PART;
            case P_VELOCITY:      return -1;
            default:              return Text::propertyDefault(id);
            }
      }

}

