#/*****************************************************************************
# * mx-findshares.pro
# *****************************************************************************
# * Copyright (C) 2014 MX Authors
# *
# * Authors: Adrian
# *          MX Linux <http://mxlinux.org>
# *
# * This program is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * MX Find Shares is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with MX Find Shares.  If not, see <http://www.gnu.org/licenses/>.
# **********************************************************************/

#-------------------------------------------------
#
# Project created by QtCreator 2014-04-02T18:30:18
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mx-findshares
TEMPLATE = app


SOURCES += main.cpp\
        mxfindshares.cpp

HEADERS  += mxfindshares.h

FORMS    += mxfindshares.ui

TRANSLATIONS += translations/mx-findshares_ca.ts \
                translations/mx-findshares_de.ts \
                translations/mx-findshares_el.ts \
                translations/mx-findshares_es.ts \
                translations/mx-findshares_fr.ts \
                translations/mx-findshares_it.ts \
                translations/mx-findshares_ja.ts \
                translations/mx-findshares_nl.ts \
                translations/mx-findshares_pl.ts \
                translations/mx-findshares_pt.ts \
                translations/mx-findshares_ro.ts \
                translations/mx-findshares_ru.ts \
                translations/mx-findshares_sv.ts \
                translations/mx-findshares_tr.ts


