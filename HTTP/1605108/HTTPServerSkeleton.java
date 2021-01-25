import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.Date;


public class HTTPServerSkeleton {
    static final int PORT = 6789;
//    public static String readFileData(File file, int fileLength) throws IOException {
//        FileInputStream fileIn = null;
//        byte[] fileData = new byte[fileLength];
//        try {
//            fileIn = new FileInputStream(file);
//            fileIn.read(fileData);
//        } finally {
//            if (fileIn != null)
//                fileIn.close();
//        }
//        return String.valueOf(fileData);
//    }
//    public static String removeSuffix(String s,String suffix)
//    {
//        if (s != null && suffix != null && s.endsWith(suffix)){
//            return s.substring(0, s.length() - suffix.length());
//        }
//        return s;
//    }

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
        //System.out.println(Arrays.toString(dir.list()));
        if (files != null && files.length > 0) {
            for (File file : files) {
                // Check if the file is a directory
                if (file.isDirectory()) {
                    // We will not print the directory name, just use it as a new
                    // starting point to list files from
                    //System.out.println("<li><a href:" + file + "> " + file + "</a></li>");
                    sb.append("<li><a href=\"/").append(file).append("\"><b>").append(file).append("</b></a></li>\r\n");
                }
                else {
                    // We can use .length() to get the file size
                    //System.out.println(file);
                    sb.append("<li><a href=\"/").append(file).append("\"> ").append(file).append("</a></li>\r\n");
                }
            }
        }
        sb.append("</ul>");
        return sb.toString();
    }
    public static void main(String[] args) throws IOException {
        ServerSocket serverConnect = new ServerSocket(PORT);
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
        while(true)
        {
            Socket s = serverConnect.accept();
            BufferedReader in = new BufferedReader(new InputStreamReader(s.getInputStream()));
            PrintWriter pr = new PrintWriter(s.getOutputStream());
            String input = in.readLine();
            if(input == null)
                continue;
            if(input.length() > 0) {
                if(input.startsWith("GET"))
                {
                    input = input.substring(4);
                    input = reverseStr(input);
                    input = input.substring(9);
                    input = reverseStr(input);
                    input = input.substring(1);
                    File file1 = new File(input);
                    pr.write("HTTP/1.1 200 OK\r\n");
                    pr.write("Server: Java HTTP Server: 1.0\r\n");
                    pr.write("Date: " + new Date() + "\r\n");
                    if(file1.isFile()){
                        InputStream inputStream = null;
                        OutputStream outputStream = null;
                        try {
                            inputStream = new BufferedInputStream(new FileInputStream(file1));
                        } catch (IOException ex) {
                            System.out.println("Can't get socket input stream.");
                        }
                        try {
                            outputStream = s.getOutputStream();
                        } catch (FileNotFoundException ex) {
                            System.out.println("File not found. ");
                        }
                        pr.write("Content-Type: application/x-force-download\r\n");
                        pr.write("Content-Length: " + file1.length() + "\r\n");
                        pr.write("\r\n");
                        pr.flush();
                        byte[] bytes = new byte[16*1024];
                        int count;
                        while ((count = inputStream.read(bytes)) >= 0) {
                            outputStream.write(bytes, 0, count);
                            outputStream.flush();
                        }
                        inputStream.close();
                        outputStream.close();
                    }
                    else
                    {
                        String flList = listFiles(input);
                        String start = "<html>\n" +
                                "\t<head>\n" +
                                "\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n" +
                                "\t</head>\n" +
                                "\t<body>";
                        String end = "</body>\n" +
                                "</html>";
                        String content = start + flList + end;
                        pr.write("Content-Type: text/html\r\n");
                        pr.write("Content-Length: " + content.length() + "\r\n");
                        pr.write("\r\n");
                        pr.write(content);
                        pr.flush();
                    }
                }
                else
                {
                    //System.out.println(input);
                    System.out.println("404: Page Not Found");
                }
            }
            s.close();
        }
    }
}
