//{{
//import yaml
//bundle = None
//with open(bundleFile) as input :
//	bundle = yaml.load(input)
//if not 'components' in bundle or bundle['components'] is None:
// 	bundle['components'] = []
//else:
//	for comp in bundle['components'] : 
//		if not 'providedServices' in comp or comp['providedServices'] is None:
//			comp['providedServices'] = []
//
//for comp in bundle['components'] :
//	if comp['name'] == componentName :
//		component = comp
//
//		for serv in comp['providedServices'] :
//			if serv['name'] == serviceName :
//				service = serv
//				break
//
//cog.outl("#ifndef __%s_H_" % service['name'].upper())
//cog.outl("#define __%s_H_" % service['name'].upper())
//cog.outl("")
//
//}}
#ifndef __EXAMPLE_H_ //do not edit, generated code
#define __EXAMPLE_H_ //do not edit, generated code
//{{end}}

//TODO add needed includes

//{{
//cog.outl("#define %s_NAME \"%s_service\"" % (service['name'].upper(), service['service_name']))
//cog.outl("#define %s_VERSION \"1.0.0.0\"" % (service['name'].upper()))
//cog.outl("")
//cog.outl("typedef struct %s_service %s;" % (service['name'], service['type']))
//cog.outl("")
//cog.outl("struct %s_service {" % service['name'])
//cog.outl("\tvoid *handle;")
//}}
#define BENCHMARK_SERVICE_NAME "benchmark_service"
typedef struct benchmark_service *benchmark_service_pt;

struct benchmark_service {
        benchmark_handler_pt handler;
//{{end}}}
        void (*callService)(void *handle, void* data);

//{{
//cog.outl("};")
//cog.outl("")
//cog.outl("")
//cog.outl("#endif /* __%s_H_ */" % service['name'])
//}}
};
#endif /* BENCHMARK_SERVICE_H_ */
//{{end}}
