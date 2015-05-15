//{{
//import json
//services = None
//service = None
//with open(descriptorFile) as input :
//	services = json.load(input)
//for serv in services :
//	if serv['name'] == serviceName :
//		service = serv
//		break
//
//cog.outl("#ifndef __%s_H_" % serviceName.upper())
//cog.outl("#define __%s_H_" % serviceName.upper())
//cog.outl("")
//
//}}
#ifndef __EXAMPLE_H_ //do not edit, generated code
#define __EXAMPLE_H_ //do not edit, generated code
//{{end}}

//TODO add needed includes

//{{
//cog.outl("#define %s_SERVICE_NAME \"%s_service\"" % (serviceName.upper(), serviceName))
//cog.outl("")
//cog.outl("typedef struct %s_service *%s_service_pt;" % (serviceName, serviceName))
//cog.outl("")
//cog.outl("struct %s_service {" % serviceName)
//cog.outl("\tvoid *handle;")
//}}
#define BENCHMARK_SERVICE_NAME "benchmark_service"
typedef struct benchmark_service *benchmark_service_pt;

struct benchmark_service {
	benchmark_handler_pt handler;
//{{end}}}

	//TODO add service methods

//{{
//cog.outl("};")
//cog.outl("")
//cog.outl("")
//cog.outl("#endif /* __%s_H_ */" % serviceName)
//}}
};
#endif /* BENCHMARK_SERVICE_H_ */
//{{end}}
