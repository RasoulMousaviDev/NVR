<script setup>
const { data, pending: fetching, refresh, clear } = useLazyFetch('/api/cameras')

const items = ref([])

watch(fetching, (v) => !v && (items.value = data.value?.items || []))

const dialog = useDialog()

const toast = useToast()

const CameraStream = defineAsyncComponent(() => import('~/components/Camera/Stream.vue'))

const handleStream = (data) => {
    dialog.open(CameraStream, { props: { modal: true, closable: true, header: data.model }, data })
}

const CameraCredentials = defineAsyncComponent(() => import('~/components/Camera/Credentials.vue'))

const handleCredentials = (data) => {
    dialog.open(CameraCredentials, { props: { modal: true, closable: true, header: data.model }, data })
}

const handleRecord = async (camera) => {

    const { pending } = useFetch(`/api/cameras/${camera.id}/record`, {
        method: 'POST',
        body: { record: +camera.record },
        onResponse({ response }) {
            if (response.ok)
                camera.record = response._data.record;
            
        },
        onResponseError({ response }) {
            setTimeout(() => {
                camera.record = 0
                toast.add({ severity: 'error', summary: 'Error', detail: response._data.message, life: 3000 })
            }, 500);
        },
        key: Date.now().toString()
    })

    camera.updating = pending
}
</script>

<template>
    <div class="bg-white rounded p-6">
        <DataTable :value="items">
            <template #header>
                <CameraHeader :fetching @refresh="[$event && clear(), refresh()]" />
            </template>
            <template #empty>
                <p class="text-center text-sm opacity-60">
                    {{ fetching ? $t('loading') : $t('not-found') }}
                </p>
            </template>
            <Column :header="$t('row')">
                <template #body="{ index }">
                    {{ index + 1 }}
                </template>
            </Column>
            <Column field="model" :header="$t('model')"></Column>
            <Column field="ip" :header="$t('ip')"></Column>
            <Column field="serial_number" :header="$t('serial-number')"></Column>
            <Column field="manufacturer" :header="$t('manufacturer')"></Column>
            <Column field="firmware_version" :header="$t('firmware-version')"></Column>
            <Column :header="$t('recording-status')">
                <template #body="{ data }">
                    <div class="flex items-center w-full">
                        <InputSwitch v-model="data.record" :true-value="1" :false-value="0"
                            :disabled="data.connect || data.updating" @value-change="handleRecord(data)" />
                    </div>
                </template>
            </Column>
            <Column :header="$t('connection-status')">
                <template #body="{ data }">
                    <Tag v-if="data.connect" severity="success" :value="$t('connected')" />
                    <Tag v-else severity="warn" :value="$t('disconnected')" />
                </template>
            </Column>
            <Column>
                <template #body="{ data }">
                    <div class="flex justify-end gap-1">
                        <Button icon="pi pi-folder" text rounded v-tooltip.top="$t('archive')"
                            @click="handleArchive(data)" />
                        <Button icon="pi pi-play-circle" text rounded :disabled="!data.connect"
                            v-tooltip.top="$t('live-stream')" @click="handleStream(data)" />
                        <Button icon="pi pi-lock" text rounded v-tooltip.top="$t('credentials')"
                            @click="handleCredentials(data)" />
                    </div>
                </template>
            </Column>
        </DataTable>
    </div>
</template>
