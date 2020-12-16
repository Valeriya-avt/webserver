# webserver
### Hello! Let's test the web server!
This server can handle GET and POST requests.
You can:
* request text files:
    - ```http://127.0.0.1:5000/index.html```
* request images:
    - ```http://127.0.0.1:5000/image.jpg```
* request grades of students in various subjects;
    - ```http://127.0.0.1:5000/get-marks?user=vasya&subject=math```
* send marks to students:
    - ```http://127.0.0.1:5000/post_form.html```
         In this case, you must fill out a special form.
