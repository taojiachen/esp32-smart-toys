idf_component_register(SRC_DIRS 
                                "."
                                "app"
                                "event"
                                
                    INCLUDE_DIRS 
                                "."
                                "app"
                                "event"
                                )
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)