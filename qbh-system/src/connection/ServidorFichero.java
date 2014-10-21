
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class ServidorFichero {

    String ficheroCepstrales;

    public static void main(String[] args) throws IOException {

        ServerSocket server;
        Socket connection;

        try {
            server = new ServerSocket(7000);
            while (true) {
                System.out.println("\n Servidor preparado.... esperando nuevo cliente.\n");
                connection = server.accept();
                // Llega un cliente.
                System.out.println("Nuevo cliente aceptado");

                new Thread(
                        new WorkerRunnable(
                        connection, "Multithreaded Server")).start();
            }
        } catch (Exception e) {
            System.err.println(e);
        }
    }
}