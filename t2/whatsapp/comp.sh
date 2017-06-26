rpcgen wpp.x

gcc wpp_svc.c wpp_clnt.c wpp_server.c wpp_xdr.c -o s_wpp -pthread

gcc wpp_client.c wpp_clnt.c wpp_xdr.c -o c_wpp