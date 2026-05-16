idf_component_register(SRCS "custom_lora.c"
                      INCLUDE_DIRS "include"
		      REQUIRES
			lora
			driver
			esp_timer
                      PRIV_REQUIRES
   			custom_comm
                        )