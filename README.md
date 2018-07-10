# wappbuilder
Simple tool, which combines various web assets (css, js, html, etc) into single web page

```
Usage: 

./wappbuild [-c][-x][-d <depfile>][-l <langfile>][-t <target>] <input.page>

<input.page>    file contains commands and references to various modules (described below)
-d  <depfile>   generated dependency file (for make)
-t  <target>    target in dependency file. If not specified, it is determined from the script
-l  <langfile>  language file (described below)
-x              do not generate output. Useful with -d (-xd depfile)
-c              collapse scripts and styles into single file(s)

Page file format

A common text file where each command is written to a separate line.
Each the line can be either empty, or contain a comment starting by #,
or can be a directive which starts with !, or a relative path to a module.

Module: One or more files where the name (without extension) is the same.
The extension specifies type of resource tied to the module
   - <name>.html : part of HTML code
   - <name>.htm : part of HTML code
   - <name>.css : part of CSS code
   - <name>.js : part of JS code
   - <name>.hdr : part of HTML code which is put to the header section
At least one file of the module must exist.

Directives

!include <file>    - include file to the script. 
                       The path is relative to the current file
!html <file>       - specify output html file (override default settins).
                       The path is relative to the root script
!css <file>        - specify output css file (override default settins).
                       The path is relative to the root script
!js <file>         - specify output js file (override default settins)
                       The path is relative to the root script
!dir <file>        - change directory of root 
                       The path is relative to the root script
!charset <cp>      - set charset of the output html page
!entry_point <fn>  - specify function used as entry point. 
                       You need to specify complete call, including ()
                       example: '!entry_point main()'

LangFile

The lang file is simple key=value file, each pair is on single line.
Comments are allowed if they are start with #
Only allowed directive is !include
Inside of page (or script), any double braced text '{{text}}' is used as key to
the lang file and if the key exists, its value is used instead.
The *.page file is also translated before it is parsed!

In source references

Inside of source files, there can be reference to other source files
You can put reference as comment using directive !require

to javascript: //!require <file.js>
to css: /*!require <file.js>*/
to html: <!--!require <file.js>-->

referenced files are also included to the project. However circular
references are not allowed resulting to break the cycle once it is detected
```
