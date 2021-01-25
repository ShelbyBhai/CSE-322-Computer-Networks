

import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.Random;

public class ServerThread implements Runnable {

    NetworkUtility networkUtility;
    EndDevice endDevice;
    int avgHopCount = 0;

    ServerThread(NetworkUtility networkUtility, EndDevice endDevice) {
        this.networkUtility = networkUtility;
        this.endDevice = endDevice;
        System.out.println("Server Ready for client " + NetworkLayerServer.clientCount);
        NetworkLayerServer.clientCount++;
        new Thread(this).start();
    }

    @Override
    public void run() {
        /**
         * Synchronize actions with client.
         */
        
        /*
        Tasks:
        1. Upon receiving a packet and recipient, call deliverPacket(packet)
        2. If the packet contains "SHOW_ROUTE" request, then fetch the required information
                and send back to client
        3. Either send acknowledgement with number of hops or send failure message back to client
        */
        networkUtility.write(endDevice);
        networkUtility.write(NetworkLayerServer.endDevices);
        int dropRateCount = 0;
        double successfulDelivary = 0;
        for (int i = 0; i <100 ; i++) {
            Packet packet = (Packet) networkUtility.read();
            if(packet.getSpecialMessage().equals("SHOW_ROUTE")){
                EndDevice sourceEndDevice = NetworkLayerServer.endDeviceMap.get( packet.getSourceIP());
                int sourceRouterID = NetworkLayerServer.deviceIDtoRouterID.get(sourceEndDevice.getDeviceID());
                Router sourceRouter = NetworkLayerServer.routerMap.get(sourceRouterID);
                sourceRouter.printRoutingTable();
                networkUtility.write(packet.getSourceIP() + "," + packet.getDestinationIP() + "," + packet.hopcount);
            }
            boolean isTest = deliverPacket(packet);
            if(isTest){
                networkUtility.write("Packet delivered");
                networkUtility.write(packet.hopcount);
                successfulDelivary++;
            }
            else {
                networkUtility.write("Not successful");
                dropRateCount++;
                //networkUtility.write(1729);
            }
        }
        //networkUtility.write(dropRateCount);
        System.out.println(dropRateCount + "%");
        if(successfulDelivary!=0)
            System.out.println(avgHopCount/successfulDelivary);
    }


    public Boolean deliverPacket(Packet p) {
        boolean test = false;
        EndDevice sourceEndDevice = NetworkLayerServer.endDeviceMap.get( p.getSourceIP());
        int sourceRouterID = NetworkLayerServer.deviceIDtoRouterID.get(sourceEndDevice.getDeviceID());
        Router sourceRouter = NetworkLayerServer.routerMap.get(sourceRouterID);

        EndDevice destinationEndDevice = NetworkLayerServer.endDeviceMap.get( p.getDestinationIP());
        int destinationRouterID = NetworkLayerServer.deviceIDtoRouterID.get(destinationEndDevice.getDeviceID());
        Router destinationRouter = NetworkLayerServer.routerMap.get(destinationRouterID);
        if(sourceRouter.getState()){
            Router intermediateRouter = sourceRouter;
           // Router intermediateRouter = Router();
            while(true){
                sourceRouter = intermediateRouter;
//                if(intermediateRouter.getRouterId() == destinationRouter.getRouterId()){
//                    test = true;
//                    break;
//                }
                if(sourceRouter.getRouterId() == destinationRouter.getRouterId()){
                    test = true;
                    break;
                }
//                if(sourceRouter.getState()){
//                    //int nexthop = sourceRouter.getRoutingTable().get(destinationRouterID-1).getGatewayRouterId();
//                    //if(nexthop == 9999){
//                      //  break;
//                    }
                    int nexthop = sourceRouter.getRoutingTable().get(destinationRouterID-1).getGatewayRouterId();
                    if(nexthop == 9999) {
                        break;
                    }

                    intermediateRouter = NetworkLayerServer.routerMap.get(nexthop);
                   // Router nextRouter = NetworkLayerServer.routerMap.get(nexthop);
                    if(!intermediateRouter.getState() && sourceRouter.getRoutingTable().get(destinationRouterID-1).getDistance()!=Constants.INFINITY ){
                        sourceRouter.getRoutingTable().get(destinationRouterID-1).setDistance(Constants.INFINITY);
                        sourceRouter.getRoutingTable().get(destinationRouterID-1).setGatewayRouterId(9999);
                        RouterStateChanger.islocked = true;
                        NetworkLayerServer.DVR(intermediateRouter.getRouterId());
                        RouterStateChanger.islocked = false;
                        break;
                    }
                    else if(intermediateRouter.getState() && sourceRouter.getRoutingTable().get(destinationRouterID-1).getDistance()==Constants.INFINITY)
                    {
                        sourceRouter.getRoutingTable().get(destinationRouterID-1).setDistance(1);
                        //sourceRouter.getRoutingTable().get(destinationRouterID-1).setGatewayRouterId();
                        RouterStateChanger.islocked = true;
                        NetworkLayerServer.DVR(intermediateRouter.getRouterId());
                        RouterStateChanger.islocked = false;
                        break;
                    }
                    p.hopcount++;
                    avgHopCount+=p.hopcount;
                }
//                else {
//                    break;
//                }
            }

        return test;
        /*
        1. Find the router s which has an interface
                such that the interface and source end device have same network address.
        2. Find the router d which has an interface
                such that the interface and destination end device have same network address.
        3. Implement forwarding, i.e., s forwards to its gateway router x considering d as the destination.
                similarly, x forwards to the next gateway router y considering d as the destination,
                and eventually the packet reaches to destination router d.

            3(a) If, while forwarding, any gateway x, found from routingTable of router r is in down state[x.state==FALSE]
                    (i) Drop packet
                    (ii) Update the entry with distance Constants.INFTY
                    (iii) Block NetworkLayerServer.stateChanger.t
                    (iv) Apply DVR starting from router r.
                    (v) Resume NetworkLayerServer.stateChanger.t

            3(b) If, while forwarding, a router x receives the packet from router y,
                    but routingTableEntry shows Constants.INFTY distance from x to y,
                    (i) Update the entry with distance 1
                    (ii) Block NetworkLayerServer.stateChanger.t
                    (iii) Apply DVR starting from router x.
                    (iv) Resume NetworkLayerServer.stateChanger.t

        4. If 3(a) occurs at any stage, packet will be dropped,
            otherwise successfully sent to the destination router
        */

    }

    @Override
    public boolean equals(Object obj) {
        return super.equals(obj); //To change body of generated methods, choose Tools | Templates.
    }
}
