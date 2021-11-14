# simple-c-webserver

### Introduction
This is a simple webserver using C which can do the following things:
1. **Display a picture** on Browser.
2. **Receive a file** uploaded from the Browser **and save** the file.

### Notice
1. The server **only supports FireFox.**
2. **Only** txt, json, csv, html, css, javascript, etc. **text files can be saved sucessfully**. 
   (That is, image files may cause the webserver to break.)

### Way to execute
`$ make`: Compile C files. <br>
`$ make exe`: Start the webserver. <br>
`$ make clean`: Remove all files produced by `$ make` instruction. <br>

### Start webserver
After compiling and execution, open browser FireFox. <br>
Type and enter URL => `https://localhost:8080/index.html`, <br>
a Pokémon will be displayed on the website; <br>
also, any acceptable file(txt, csv, json... etc.) can be uploaded and saved in the server.
