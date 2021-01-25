import java.io.*;
import java.net.Socket;
import java.util.Date;

public class Worker extends Thread {
        Socket socket;
        public Worker(Socket socket)
        {
            this.socket = socket;
        }
        public void run()
        {
            try {
                BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                PrintWriter pr = new PrintWriter(socket.getOutputStream());
                String input = in.readLine();
//                if(input == null) {
//                    continue;
//                }
                if(input.length() > 0) {
                    if (input.startsWith("GET")) {
                        input = input.substring(4);
                        input = Server.reverseStr(input);
                        input = input.substring(9);
                        input = Server.reverseStr(input);
                        input = input.substring(1);
                        File file1 = new File(input);
                        pr.write("HTTP/1.1 200 OK\r\n");
                        pr.write("Server: Java HTTP Server: 1.0\r\n");
                        pr.write("Date: " + new Date() + "\r\n");
                        if (file1.isFile()) {
                            InputStream inputStream = null;
                            OutputStream outputStream = null;
                            try {
                                inputStream = new BufferedInputStream(new FileInputStream(file1));
                                //inputStream = socket.getInputStream();
                            } catch (IOException ex) {
                                System.out.println("Can't get socket input stream.");
                            }
                            try {
                                outputStream = socket.getOutputStream();
                                //outputStream = new BufferedOutputStream(new FileOutputStream(file1));
                            } catch (FileNotFoundException ex) {
                                System.out.println("File not found. ");
                            }
                            pr.write("Content-Type: application/x-force-download\r\n");
                            pr.write("Content-Length: " + file1.length() + "\r\n");
                            pr.write("\r\n");
                            pr.flush();
                            byte[] bytes = new byte[16 * 1024];
                            int count;
                            while ((count = inputStream.read(bytes)) >= 0) {
                                outputStream.write(bytes, 0, count);
                                outputStream.flush();
                            }
                            inputStream.close();
                            outputStream.close();
                        } else {
                            String flList = Server.listFiles(input);
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
                    else if(input.startsWith("PUT")){
                        String path = "D:\\3.2\\CSE321+322(NETWORK)\\LAB\\Offlines\\Offline1";
                        FileOutputStream fos = new FileOutputStream(path);
                        BufferedOutputStream out = new BufferedOutputStream(fos);
                        byte[] buffer = new byte[1024];
                        int count;
                        InputStream inputStream = socket.getInputStream();
                        while((count=inputStream.read(buffer)) >0){
                            fos.write(buffer);
                        }
                        fos.close();
                    }
                    else {
                        //System.out.println(input);
                        System.out.println("404: Page Not Found");
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
//            catch (IOException | InterruptedException e) {
//                e.printStackTrace();
//            }
        }
}
