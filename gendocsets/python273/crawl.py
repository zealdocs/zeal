import sqlite3
import os

from lxml.html import parse

os.unlink('index.sqlite')
conn = sqlite3.connect('index.sqlite')
c = conn.cursor()

c.execute('CREATE TABLE things (id integer primary key, type text, name text, path text, parent integer)')

tree = parse('py-modindex.html')

modules = {}
classes = {}

for tbl in tree.xpath('//table[@class="indextable modindextable"]'):
    for tr in tbl.findall('tr'):
        a = tr.findall('td')[1].find('a')
        if a is None: continue
        modname = a.find('tt').text
        c.execute('INSERT INTO things(type, name, path) values("module", ?, ?)', (modname, a.attrib['href']))
        modules[modname] = (c.lastrowid, a.attrib['href'].split('#')[0])


parsed_files = set()
for modname, (modid, fname) in modules.items():
    if '.' in modname: modname = modname.split('.')[0]  # modules aren't well-structured
    if fname in parsed_files: continue
    parsed_files.add(fname)
    tree = parse(fname)
    # classes/exceptions
    for cls in tree.xpath('//dl[@class="class" or @class="exception"]'):
        header = cls.find('dt')
        url = fname
        if 'id' not in header.attrib:
            continue
        url += '#' + header.attrib['id']
        classname = cls.xpath('dt/tt[@class="descname"]/text()')[0]
        modname_cls = cls.xpath('dt/tt[@class="descclassname"]/text()')
        if modname_cls:
            modname_cls = modname_cls[0][:-1]
        else:
            modname_cls = modname
        if modname_cls not in modules:
            c.execute('INSERT INTO things(type, name, path) values("module", ?, ?)', (modname_cls, fname))
            modules[modname_cls] = (c.lastrowid, fname)
        c.execute('INSERT INTO things(type, name, path, parent) values(?, ?, ?, ?)', 
            (cls.attrib['class'], classname, url, modules[modname_cls][0]))
        classes[classname] = c.lastrowid

    # methods/functions:
    for method in tree.xpath('//dl[@class="method" or @class="function"]'):
        classname = method.xpath('dt/tt[@class="descclassname"]/text()')
        if classname: classname = classname[0][:-1]
        methodname = method.xpath('dt/tt[@class="descname"]/text()')[0]
        if not classname:
            dl = [a for a in method.iterancestors() if a.attrib.get('class') == 'class']
            if dl:
                classname = dl[0].xpath('dt/tt[@class="descname"]/text()')
                if classname: classname = classname[0]

        url = fname
        if ' ' in methodname:
            # ignore some weird cases from deprecated SGI/SunOS modules
            assert ' SGI ' in open(fname).read() or ' SunOS ' in open(fname).read()
            continue

        if method.xpath('dt/@id'):
            url += '#'+method.xpath('dt/@id')[0]
        else:
            # should we update html file to contain id?
            # there are only 23 such cases (at time of development), so might be not worth it...
            pass

        if not classname or classname == modname:
            type_ = "function"
            parentid = modid
        else:
            type_ = "member"
            if classname not in classes:
                c.execute('INSERT INTO things(type, name, path, parent) values("class", ?, ?, ?)', 
                    (classname, url, modid))
                classes[classname] = c.lastrowid
            parentid = classes[classname]
        
        c.execute('INSERT INTO things(type, name, path, parent) values("%s", ?, ?, ?)' % type_,
            (methodname, url, parentid))


std_classes = {}
def get_std_class(name, href):
    if name not in std_classes:
        c.execute('INSERT INTO things(type, name, path) values("class", ?, ?)', 
            (name, href))
        std_classes[name] = c.lastrowid
    return std_classes[name]


tree = parse('library/stdtypes.html')
for method in tree.xpath('//dl[@class="method"]'):
    url = 'library/stdtypes.html'
    if method.xpath('dt/@id'):
        url += '#'+method.xpath('dt/@id')[0]
    classname = method.xpath('dt/tt[@class="descclassname"]/text()')
    if classname:
        classname = classname[0][:-1]
    else:
        classname = method.getparent().getparent().find('dt').find('tt').text
    c.execute('INSERT INTO things(type, name, path, parent) values("member", ?, ?, ?)',
        (method.xpath('dt/tt[@class="descname"]/text()')[0], url,
         get_std_class(classname, url)))


tree = parse('library/functions.html')
for method in tree.xpath('//dl[@class="function"]'):
    url = 'library/functions.html'
    if method.xpath('dt/@id'):
        url += '#'+method.xpath('dt/@id')[0]
    c.execute('INSERT INTO things(type, name, path) values("function", ?, ?)',
        (method.xpath('dt/tt[@class="descname"]/text()')[0], url))
         

conn.commit()
conn.close()
