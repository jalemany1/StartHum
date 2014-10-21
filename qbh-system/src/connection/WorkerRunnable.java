
/**
 *
 * @author emilio
 */
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.BufferedWriter;
import java.net.Socket;

public class WorkerRunnable implements Runnable {

    protected Socket connection = null;
    protected String serverText = null;
  
    String root = "../../";
    String androidID;

    public WorkerRunnable(Socket connection, String serverText) {
        this.connection = connection;
        this.serverText = serverText;
    }

    public void run() {
        try {
            // Cuando se cierre el socket, esta opción hará que el cierre se
            // retarde automáticamente hasta 10 segundos dando tiempo al cliente
            // a leer los datos.
            connection.setSoLinger(true, 10);
  
            byte[] receivedData = new byte[1024];
            BufferedInputStream bis = new BufferedInputStream(connection.getInputStream());
            DataInputStream dis = new DataInputStream(connection.getInputStream());
            DataOutputStream dos = new DataOutputStream(connection.getOutputStream());

            //recibimos la ID del dispositivo
            androidID = dis.readUTF();
            System.out.println("Nombre del dispositivo: "+androidID);
          
            //recibimos el fichero de audio
            int size = dis.readInt();
            FileOutputStream fos = new FileOutputStream(root+"media/temp/"+androidID+".wav");
            for (int i = 0; i < size;) {
                int in = bis.read(receivedData);
                fos.write(receivedData, 0, in);
                i += in;
            }
            fos.close();
            System.out.println("Fichero recibido!");

            //realizamos el matching
            try {
                Process p1 = Runtime.getRuntime().exec(root+"build/melody -i "+root+"media/temp/"+androidID+".wav -o "+root+"media/temp/"+androidID);
                p1.waitFor();
                Process p2 = Runtime.getRuntime().exec(root+"build/matching "+root+"media/temp/"+androidID+" -o "+root+"media/temp/"+androidID+".xml -m dtw");
                p2.waitFor();
            } catch (Exception e) {
              e.printStackTrace();
            }
          
            //enviamos el fichero XML resultado del matching
            BufferedOutputStream bos = new BufferedOutputStream(connection.getOutputStream());
            File transferFile = new File(root+"media/temp/"+androidID+".xml");
            if (transferFile.isFile()) {
                FileInputStream fis = new FileInputStream(transferFile);
                // send size
                dos.writeInt((int) transferFile.length());
                dos.flush();
                // send file
                int read;
                byte[] buf = new byte[1024];
                while ((read = fis.read(buf, 0, 1024)) != -1) {
                    bos.write(buf, 0, read);
                    bos.flush();
                }
                fis.close();
            }
            System.out.println("Fichero enviado!");
          
            connection.close();
        } catch (IOException e) {
            //report exception somewhere.
            e.printStackTrace();
        }
    }

}