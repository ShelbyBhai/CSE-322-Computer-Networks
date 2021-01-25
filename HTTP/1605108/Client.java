import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Scanner;

public class Client {
    public static void main(String[] args) throws IOException {
        ServerSocket server_socket = new ServerSocket(6789);
        Scanner scanner = new Scanner(System.in);
        String string = scanner.nextLine();
        File myFile = new File(string);
        Socket socket = server_socket.accept();
        int count;
        byte[] buffer = new byte[1024];
        OutputStream out = socket.getOutputStream();
        BufferedInputStream in = new BufferedInputStream(new FileInputStream(myFile));
        while ((count = in.read(buffer)) > 0) {
            out.write(buffer, 0, count);
            out.flush();
        }
    }
}
