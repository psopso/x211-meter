idf_component_register(SRCS "custom_comm.c"
                      INCLUDE_DIRS "include"
			PRIV_REQUIRES 
              		  custom_config 
              		  custom_wifi
			  custom_mqtt
			  custom_time
			  custom_ota
			  custom_lora
                      	REQUIRES 
			  my_debug
			  custom_dlms
			  custom_storage
)