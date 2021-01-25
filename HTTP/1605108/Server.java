import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Server {
    static final int PORT = 6789;
    public static String reverseStr(String s)
    {
        String reverse = "";
        for(int i = s.length() - 1; i >= 0; i--)
        {
            reverse = reverse + s.charAt(i);
        }
        return reverse;
    }
    public static String listFiles(String startDir) {
        File dir = new File(startDir);
        File[] files = dir.listFiles();
        StringBuilder sb = new StringBuilder();
        sb.append("<ul>");
        if (files != null && files.length > 0) {
            for (File file : files) {
                // Check if the file is a directory
                if (file.isDirectory()) {
                    // We will not print the directory name, just use it as a new
                    // starting point to list files from
                    sb.append("<li><a href=\"/").append(file).append("\"><b>").append(file).append("</b></a></li>\r\n");
                }
                else {
                    // We can use .length() to get the file size
                    sb.append("<li><a href=\"/").append(file).append("\"> ").append(file).append("</a></li>\r\n");
                }
            }
        }
        sb.append("</ul>");
        return sb.toString();
    }
    public static void main(String[] args) throws IOException, ClassNotFoundException {
        ServerSocket welcomeSocket = new ServerSocket(PORT);
        System.out.println("Server started.\nListening for connections on port : " + PORT + " ...\n");
        File file = new File("index.html");
        FileInputStream fis = new FileInputStream(file);
        BufferedReader br = new BufferedReader(new InputStreamReader(fis, StandardCharsets.UTF_8));
        StringBuilder sb = new StringBuilder();
        String line;
        while(( line = br.readLine()) != null ) {
            sb.append( line );
            sb.append( '\n' );
        }
        while (true) {
            Socket socket = welcomeSocket.accept();
            // open thread
            Thread worker = new Worker(socket);
            worker.start();
            //socket.close();
        }
    }
}
