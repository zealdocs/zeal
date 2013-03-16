#!/usr/bin/env python

"""ExtJS docsets parser - needs to be run in the ExtJS reference documentation
directory as a working directory. It can be downloaded from http://docs.sencha.com/"""

from cStringIO import StringIO
import errno
import os
import re
from json import loads
from lxml.html import parse, tostring
from shutil import copytree, rmtree, copy

import sqlite3

INPUT_DIR = '.'
OUTPUT_DIR = os.path.join(INPUT_DIR, 'output')
OUT_DIR = 'ExtJS-4.1.docset'
DOCUMENTS_DIR = os.path.join(OUT_DIR, 'Contents', 'Resources', 'Documents')
HTML_DIR = os.path.join(DOCUMENTS_DIR, 'html')

rmtree(OUT_DIR, ignore_errors=True)
copytree(INPUT_DIR, DOCUMENTS_DIR)

# index.html doesn't work with Dash
rmtree(os.path.join(DOCUMENTS_DIR, 'output'))
rmtree(os.path.join(DOCUMENTS_DIR, 'extjs-build'))
rmtree(os.path.join(DOCUMENTS_DIR, 'guides'))
os.unlink(os.path.join(DOCUMENTS_DIR, 'index.html'))

copy(os.path.join(os.path.dirname(__file__), 'icon.png'), OUT_DIR)
os.mkdir(HTML_DIR)

with open(os.path.join(OUT_DIR, 'Contents', 'Info.plist'), 'w') as plist:
    plist.write("""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleIdentifier</key>
	<string>ExtJS 4.1</string>
	<key>CFBundleName</key>
	<string>ExtJS 4.1</string>
	<key>DocSetPlatformFamily</key>
	<string>ExtJS 4.1</string>
	<key>isDashDocset</key>
	<true/>
</dict>
</plist>""")

conn = sqlite3.connect(os.path.join(OUT_DIR, 'Contents', 'Resources', 'docSet.dsidx'))
c = conn.cursor()
c.execute('CREATE TABLE searchIndex (id integer primary key, name text, type text, path text)')

ids = {}
trees = {}

h3_bug_re = re.compile(r"<h4 class='members-subtitle'>([^<>]+)</h3>")

for fname in os.listdir(OUTPUT_DIR):
    data = loads(open(os.path.join(OUTPUT_DIR, fname), 'r').read().split('(', 1)[1].rsplit(')', 1)[0])
    html = h3_bug_re.sub('<h4 class="members-subtitle">\1</h4>', data['html'].encode('utf-8'))
    name = fname.rsplit('.', 1)[0]+'.html'
    trees[name] = tree = parse(StringIO(html))
    for div in tree.findall('//div'):
        if 'id' in div.attrib:
            ids[name[:-len('.html')]+'-'+div.attrib['id']] = (name, div.attrib['id'])

untypes = set()
for fname, tree in trees.iteritems():
    with open(os.path.join(HTML_DIR, fname), 'w') as htmlfile:
        htmlfile.write("""<!doctype html>
<html>
<head>
    <link rel="stylesheet" href="../resources/css/app-851b66f18114a2a31d488e9c8d7cb964.css" type="text/css" />
    <style>
        body {
            margin: 2em;
        }
        .members .member .short {
            display: none;
        }
        .members .member .long {
            display: block;
        }
    </style>
</head>
<body><div class="class-overview"><div class="x-panel-body">""")
        
        for a in tree.findall('//a'):
            if 'href' not in a.attrib: continue
            if a.attrib['href'].startswith('source/'):
                a.attrib['href'] = '../' + a.attrib['href']
            else:
                if not a.attrib['href'].startswith('#'): continue
                if '/guide/' in a.attrib['href'] or '/example' in a.attrib['href']: continue
                
                fragment = a.attrib['href'].replace('#!/api/', '')
                def cfg():
                    return fragment.replace('property-', 'cfg-')
                
                if fragment in ids:
                    a.attrib['href'] = ids[fragment][0] + '#' + ids[fragment][1]
                elif cfg() in ids:
                    a.attrib['href'] = ids[cfg()][0] + '#' + ids[cfg()][1]
                elif fragment == '#':
                    # remove JS elements
                    a.getparent().remove(a)
                else:
                    if os.path.isfile(os.path.join(OUTPUT_DIR, fragment+'.js')):
                        a.attrib['href'] = fragment+'.html'
                    else:
                        print fragment, 'missing.'
        
        for pre in tree.findall('//pre'):
            if not pre.attrib.get('class'):
                # enables CSS
                pre.attrib['class'] = 'notpretty'

        for section in tree.getroot().find_class('members-section'):
            stype = section.find('h3').text
            try:
                idxtype = {'Methods': 'clm',
                           'Properties': 'Attribute',
                           'Config options': 'Attribute',
                           'Events': 'event',
                           'CSS Mixins': 'func',
                           'CSS Variables': 'var'}[stype]
            except KeyError:
                untypes.add(stype)
                idxtype = 'unknown'
            for member in section.find_class('member'):
		if member.find_class('defined-in')[0].text != fname[:-len('.html')]:
		    assert member.find_class('defined-in')[0].text + '.html' in trees, member.find_class('defined-in')[0].text
		    continue
                c.execute('INSERT INTO searchIndex(type, name, path) values(?, ?, ?)',
                        (idxtype, fname[:-len('html')]+member.xpath('div[@class="title"]/a/text()')[0],
                         os.path.join('html', fname)+'#'+member.attrib['id']))
        
        c.execute('INSERT INTO searchIndex(type, name, path) values("cl", ?, ?)',
                    (fname[:-len('.html')], os.path.join('html', fname)))
        
        htmlfile.write(tostring(tree))
        
        htmlfile.write("""</body>
</div></div></html>""")
            
conn.commit()
conn.close()
if untypes:
    print 'Unknown types:', untypes
