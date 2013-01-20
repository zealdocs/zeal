import sqlite3
import os

from lxml.html import parse

os.unlink('index.sqlite')
conn = sqlite3.connect('index.sqlite')
c = conn.cursor()

c.execute('CREATE TABLE things (id integer primary key, type text, name text, path text, parent integer)')

tree = parse('qtdoc/modules.html')

for tbl in tree.xpath('//table[@class="generic"]'):
    for tr in tbl.findall('tr'):
        a = tr.find('td').find('a')
        c.execute('INSERT INTO things(type, name, path) values("module", ?, ?)', (a.text, a.attrib['href'].replace('../', '')))


def parseClass(class_id, path):
    tree = parse(path)
    basepath = path.split('/')[0]
    for a in tree.xpath('//td[@class="memItemRight bottomAlign"]/b/a'):
        c.execute('INSERT INTO things(type, name, path, parent) values("member", ?, ?, ?)', (a.text, basepath+'/'+a.attrib['href'], class_id))


tree = parse('qtdoc/classes.html')
for cls in tree.xpath('//dd'):
    a = cls.find('a')
    url = a.attrib['href'].replace('../', '')
    if not url.startswith('http://'):
        c.execute('INSERT INTO things(type, name, path) values("class", ?, ?)', (a.text, url))
        parseClass(c.lastrowid, url)


# global functions:
tree = parse('qtdoc/functions.html')
for fun in tree.xpath('//li'):
    if fun.find('a') is None: continue
    if fun.find('a').text == 'global':
        url = fun.find('a').attrib['href'].replace('../', '')
        c.execute('INSERT INTO things(type, name, path) values("function", ?, ?)',
            (fun.text.strip(': '), url))


conn.commit()
conn.close()
