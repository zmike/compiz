#include "e_mod_main.h"

EAPI E_Module_Api e_modapi = {E_MODULE_API_VERSION, "Compiz"};
static E_Config_DD *conf_edd = NULL;

EINTERN Mod *compiz_mod = NULL;
EINTERN Config *compiz_config = NULL;

#if 0
static void
_e_mod_ds_config_load(void)
{
#undef T
#undef D
   conf_edd = E_CONFIG_DD_NEW("Config", Config);
   #define T Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, config_version, UINT);
   E_CONFIG_VAL(D, T, disable_ruler, UCHAR);
   E_CONFIG_VAL(D, T, disable_maximize, UCHAR);
   E_CONFIG_VAL(D, T, disable_transitions, UCHAR);
   E_CONFIG_VAL(D, T, disabled_transition_count, UINT);

   E_CONFIG_VAL(D, T, types.disable_PAN, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_FADE_OUT, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_FADE_IN, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_BATMAN, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_ZOOM_IN, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_ZOOM_OUT, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_GROW, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_ROTATE_OUT, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_ROTATE_IN, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_SLIDE_SPLIT, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_QUAD_SPLIT, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_QUAD_MERGE, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_BLINK, UCHAR);
   E_CONFIG_VAL(D, T, types.disable_VIEWPORT, UCHAR);

   ds_config = e_config_domain_load("module.compiz", conf_edd);
   if (ds_config)
     {
        if (!e_util_module_config_check("Compiz", ds_config->config_version, MOD_CONFIG_FILE_VERSION))
          E_FREE(ds_config);
     }

   if (!ds_config)
     ds_config = E_NEW(Config, 1);
   ds_config->config_version = MOD_CONFIG_FILE_VERSION;
}
#endif
EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   snprintf(buf, sizeof(buf), "%s/e-module-compiz.edj", m->dir);

   compiz_mod = E_NEW(Mod, 1);
   compiz_mod->module = m;
   compiz_mod->edje_file = eina_stringshare_add(buf);
   compiz_init();

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m EINA_UNUSED)
{
   compiz_shutdown();
   //e_config_domain_save("module.compiz", conf_edd, compiz_config);
   E_CONFIG_DD_FREE(conf_edd);
   eina_stringshare_del(compiz_mod->edje_file);
   E_FREE(compiz_mod);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m EINA_UNUSED)
{
   //e_config_domain_save("module.compiz", conf_edd, ds_config);
   return 1;
}
