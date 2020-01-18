from PyQt5.QtCore import *
from PyQt5.QtWidgets import *
import socket
import sys

HOST = "127.0.0.1"
PORT = 5555

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server_address = (HOST, PORT)
sock.connect(server_address)

app = QApplication([])
text_area = QPlainTextEdit()
text_area.setFocusPolicy(Qt.NoFocus)
message = QLineEdit()
layout = QVBoxLayout()
layout.addWidget(text_area)
layout.addWidget(message)
window = QWidget()
window.setLayout(layout)
window.show()

def display_response(response):
    text_area.clear()
    print(response)
    text_area.appendPlainText(response)

def send_request():
    sock.sendall(message.text().encode())
    response = sock.recv(1024).decode('utf-8')
    display_response(response)
    message.clear()
    

message.returnPressed.connect(send_request)

app.exec_()