idf_component_register(SRCS "custom_storage.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES custom_dlms
		      REQUIRES custom_config )