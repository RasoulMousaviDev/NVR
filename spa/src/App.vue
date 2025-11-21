<script setup>
import Toast from 'primevue/toast'
import DynamicDialog from 'primevue/dynamicdialog'
import { provide, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useToast } from 'primevue/usetoast'
import { useRoute, useRouter } from 'vue-router'
import { useDialog } from 'primevue/usedialog'

const { t } = useI18n()
const toast = useToast()
const route = useRoute()
const router = useRouter()
const dialog = useDialog()

const services = { toast, dialog, router, route, t }

provide('services', services)
</script>

<template>
  <RouterView />
  <DynamicDialog />
  <Toast position="bottom-left"/>
</template>
