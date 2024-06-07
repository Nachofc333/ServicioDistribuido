from enum import Enum
import argparse
import socket
import threading
import os
import zeep
import re
import shutil

class client :
    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2
        # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _socket = None
    _listen_socket = None
    _listen_thread = None
    _user = None
    HILO = True
    _hilo_lock = threading.Lock()  # Mutex para proteger HILO

    wsdl_url = "http://localhost:8000/?wsdl"
    soap = zeep.Client(wsdl=wsdl_url)
    
    def time():
        return client.soap.service.time_sv()
    # ******************** METHODS *******************
    @staticmethod
    def handle_requests():
        while True:
            with client._hilo_lock:
                if not client.HILO:
                    break
            
            try:
                if client._listen_socket.fileno() != -1:  # Verifica si el socket está abierto
                    conn, addr = client._listen_socket.accept()
                # maneja la solicitud aquí
                data = conn.recv(1024).decode()
                data = data.split(' ')
                if data[0] == 'F':
                    file_name = data[1].strip('\0')
                    if os.path.exists(f"{client._user}/{file_name}"):
                        with open(f"{client._user}/{file_name}", 'rb') as file:
                            while True:
                                data = file.read(1024)
                                if not data:
                                    break
                                conn.sendall(data)
                else:
                    conn.sendall("1\0".encode())  # indica que el archivo no existe
                conn.close()
            except socket.timeout:
                continue
    @staticmethod
    def register(user):
        time = client.time()
        try:
            _socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            _socket.connect((client._server, client._port))
            message = f"REGISTER {time} {user}\0"
            _socket.sendall(message.encode())
            response = _socket.recv(1024).decode()
            _socket.close()
            if response == '0':
                print("c > REGISTER OK")
                os.makedirs(user, exist_ok=True)  # crea un directorio para el usuario
                return client.RC.OK
            elif response == '1':
                print("c > USERNAME IN USE")
                return client.RC.USER_ERROR
            else:
                print("c > REGISTER FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR

        
    @staticmethod
    def unregister(user):
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"UNREGISTER {time} {user}\0".encode())
            response = client._socket.recv(1024).decode()
            if response == '0':
                print("c > UNREGISTER OK")
                shutil.rmtree(user)
                return client.RC.OK
            elif response == '1':
                print("c > USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                print("c > UNREGISTER FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
            
    @staticmethod
    def connect(user):
        if client._user is not None:
            print("c > USER ALREADY CONNECTED")
            return client.RC.USER_ERROR
        time = client.time()
        try:
            client._user = user
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._listen_socket.bind(('localhost', 0))  # bind a un puerto libre
            listen_port = client._listen_socket.getsockname()[1]
            listen_ip = client._listen_socket.getsockname()[0]
            client._listen_socket.settimeout(1)  # establece un tiempo de espera de 1 segundo
            client._listen_socket.listen(1)
            client._listen_thread = threading.Thread(target=client.handle_requests)
            client._listen_thread.start()
            client._socket.sendall(f"CONNECT {time} {user} {listen_ip} {listen_port}\0".encode())
            response = client._socket.recv(1024).decode()
            if response == '0':
                print("c > CONNECT OK")
                return client.RC.OK
            elif response == '1':
                print("c > CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
                print("c > CONNECT FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
    @staticmethod
    def disconnect(user):
        if client._user is None:
            print("c > DISCONNECT FAIL, USER NOT CONNECTED")
            return client.RC.USER_ERROR
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"DISCONNECT {time} {user}\0".encode())
            response = client._socket.recv(1024).decode()
            if response == '0':
                client._user = None
                with client._hilo_lock:
                    client.HILO = False
                print("c > DISCONNECT OK")
                client._listen_thread.join()  # para el hilo de escucha
                
                client._listen_socket.close()  # cierra el socket de escucha
                return client.RC.OK
            elif response == '1':
                print("c > DISCONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > DISCONNECT FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                print("c > DISCONNECT FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
    @staticmethod
    def publish(fileName, description):
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"PUBLISH {time} {client._user} {fileName} {description}\0".encode())
            response = client._socket.recv(1024).decode()
            if response == '0':
                print("c > PUBLISH OK")
                os.mknod(f"{client._user}/{fileName}")  # crea un archivo vacío en el directorio del usuario
                return client.RC.OK
            elif response == '1':
                print("c > PUBLISH FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > PUBLISH FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            elif response == '3':
                print("c > PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
                return client.RC.USER_ERROR
            else:
                print("c > PUBLISH FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
    @staticmethod
    def delete(fileName):
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"DELETE {time} {client._user} {fileName}\0".encode())
            response = client._socket.recv(1024).decode()
            if response == '0':
                print("c > DELETE OK")
                os.remove(f"{client._user}/{fileName}")
                return client.RC.OK
            elif response == '1':
                print("c > DELETE FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > DELETE FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            elif response == '3':
                print("c > DELETE FAIL, CONTENT NOT PUBLISHED")
                return client.RC.USER_ERROR
            else:
                print("c > DELETE FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
    @staticmethod
    def listusers():
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"LIST_USERS {time} {client._user}\0".encode())
            response = client._socket.recv(1024).decode()
            if response[0] == '0':
                num_users = int(response[1])
                print("c > LIST_USERS OK")
                user_info_list = response[2:].split('\0')
                user_info = user_info_list[0]
                user_info = re.split(' ', user_info)
                for i in range(num_users): 
                    print(f"{user_info[i]} {user_info[i+1]} {user_info[i+2]}")
                    user_info.remove(user_info[i])
                    user_info.remove(user_info[i+1])
                return client.RC.OK
            elif response == '1':
                print("c > LIST_USERS FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > LIST_USERS FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                print("c > LIST_USERS FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
    @staticmethod
    def listcontent(user):
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))
            client._socket.sendall(f"LIST_CONTENT {time} {user} {client._user}\0".encode())
            response = client._socket.recv(2048).decode()
            if response[0] == '0':
                response = response[1:]
                pattern = r"(\w+\.\w+)"  # busca palabras que contengan un punto (nombres de archivo)
                matches = re.finditer(pattern, response)
                start = 0
                for match in matches:
                    description = response[start:match.start()].strip()
                    if description:
                        print(f'{file_name} "{description}"')
                    file_name = match.group()
                    start = match.end()
                description = response[start:].strip()
                if description:
                    print(f'{file_name} "{description}"')
                return client.RC.OK
            elif response == '1':
                print("c > LIST_CONTENT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif response == '2':
                print("c > LIST_CONTENT FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            elif response == '3':
                print("c > LIST_CONTENT FAIL, REMOTE USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                print("c > LIST_CONTENT FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()

    @staticmethod
    def getfile(user, remote_file_name, local_file_name):
        time = client.time()
        try:
            client._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client._socket.connect((client._server, client._port))  
            client._socket.sendall(f"GET_FILE {time} {client._user} {user} {remote_file_name}\0".encode())
            response = client._socket.recv(1024).decode()
            if response[0] == '0':
                print("c > GET_FILE OK")
                # crear una conexión con el otro cliente
                response = response.split(' ')
                file_ip = response[1]
                file_port = int(response[2].strip('\x00'))
                file_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                file_socket.connect((file_ip, file_port))
                file_socket.sendall(f"F {remote_file_name}\0".encode())
                with open(f"{client._user}/{local_file_name}", 'w') as file:
                    while True:
                        data = file_socket.recv(1024).decode()
                        if not data:
                            break
                        file.write(data)

                return client.RC.OK
            elif response == '1':
                print("c > GET_FILE FAIL, FILE NOT EXIST")
                return client.RC.USER_ERROR
            else:
                print("c > GET_FILE FAIL")
                return client.RC.ERROR
        except Exception as e:
            print(f"Error: {str(e)}")
            return client.RC.ERROR
        finally:
            client._socket.close()
            if response[0] != '0' and os.path.exists(local_file_name):
                os.remove(local_file_name)  # delete the local file if the transfer was not successful
    @staticmethod
    def parseArguments(argv):
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()
        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False
        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")
            return False
        client._server = args.s
        client._port = args.p
        return True
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c > ")
                line = command.split(" ")
                if (len(line) > 0):  
                    line[0] = line[0].upper()
                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")
                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")
                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")
                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")
                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")
                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")
                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")
                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")
                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            if client._user is not None:
                                client.disconnect(client._user)
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))
    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")
    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()
        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False
        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p
        return True
    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return
        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    
if __name__=="__main__":
    client.main([])