<script setup>
import Toast from 'primevue/toast'
import DynamicDialog from 'primevue/dynamicdialog'
import ConfirmDialog from 'primevue/confirmdialog';
import Popover from 'primevue/popover';
import { onMounted, provide, ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import { useToast } from 'primevue/usetoast'
import { useRoute, useRouter } from 'vue-router'
import { useDialog } from 'primevue/usedialog'
import { useConfirm } from "primevue/useconfirm";

const { t } = useI18n()
const toast = useToast()
const route = useRoute()
const router = useRouter()
const dialog = useDialog()
const confirm = useConfirm();
const popover = ref()

const services = { toast, dialog, confirm, popover, router, route, t }

provide('services', services)
</script>

<template>
  <RouterView />
  <DynamicDialog />
  <Toast />
</template>
