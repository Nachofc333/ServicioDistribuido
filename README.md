# p_final_ssdd

Para compilar el código en C ejecutamos el siguiente comando: 
    make -f Makefile.imprimir

Para iniciar el servidor RPC ejecutamos los dos siguientes comandos:
    sudo mkdir -p /run/sendsigs.omit.d/
    sudo /etc/init.d/rpcbind restart

Cada ejecución se tiene que hacer en una terminal diferente.

Para ejecutar el servicio web en python ejecutamos el siguiente comando:
    python3 servicio_web.py & python3 -mzeep http://localhost:8000/?wsdl

Para ejecutar el servidor RPC:
    ./imprimir_server

Para ejecutar el servidor:
Primero declarar una variable de entorno con la ip del servidor rpc: 
    export IMPRIMIR_SERVER=localhost
Ejecutar el servidor:
    ./servidor -p 8080

Para ejecutar el cliente, se puede ejecutar varios clientes en terminales diferentes:
    python3 ./client.py -s localhost -p 8080

Para simular el funcionamiento de un servidor real y poder visualizarlo, hemos implemntado que por cada usuario que se registra, se crea un directoria asociado a él y es donde se localizan sus archivos.