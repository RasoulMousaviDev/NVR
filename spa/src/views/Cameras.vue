<script setup>
const { dialog, toast, router, t } = inject('services');

const store = useCameraStore()
// store.index()

const menu = ref();
const options = ref();

const CameraStream = defineAsyncComponent(() => import('@/components/Camera/Stream.vue'))
const CameraSettings = defineAsyncComponent(() => import('@/components/Camera/Settings.vue'))
const CameraCredentials = defineAsyncComponent(() => import('@/components/Camera/Credentials.vue'))

const toggle = (data, event) => {
    const props = { modal: true, closable: true, header: data.model }
    const disabled = !data.connect

    options.value = [
        {
            label: t('archive'), icon: 'pi pi-folder',
            command: () => router.push(`/videos?camera=${data.model}`)
        },
        {
            label: t('live-stream'), icon: 'pi pi-play-circle', disabled,
            command: async () => {
                const url = `/api/cameras/${data.id}/stream?t=${Date.now()}`

                const { status, error } = await useFetch(url)

                if (status.value == 'success')
                    return dialog.open(CameraStream, { props, data: { url } })

                toast.add({ severity: 'error', summary: 'Error', detail: error.value.data.message, life: 3000 })

            }
        },
        {
            label: t('settings'), icon: 'pi pi-cog', disabled,
            command: () => dialog.open(CameraSettings, { props, data })
        },
        {
            label: t('authentication'), icon: 'pi pi-lock', disabled,
            command: () => dialog.open(CameraCredentials, { props, data })
        },
    ]

    menu.value.toggle(event);
};

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
        <DataTable :value="store.items">
            <template #header>
                <CameraHeader />
            </template>
            <template #empty>
                <p class="text-center text-sm opacity-60">
                    {{ store.fetching ? $t('loading') : $t('not-found') }}
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
            <Column :header="$t('recording')">
                <template #body="{ data }">
                    <div class="flex items-center w-full">
                        <InputSwitch v-model="data.record" :true-value="1" :false-value="0"
                            :disabled="!data.connect || data.updating" @value-change="handleRecord(data)" />
                    </div>
                </template>
            </Column>
            <Column :header="$t('connection-status')">
                <template #body="{ data }">
                    <Tag v-if="data.connect" severity="success" :value="$t('connected')" />
                    <Tag v-else severity="warn" :value="$t('disconnected')" />
                </template>
            </Column>
            <Column body-class="text-left pl-0">
                <template #body="{ data }">
                    <Button icon="pi pi-ellipsis-v" text rounded @click="toggle(data, $event)" />
                </template>
            </Column>
        </DataTable>
        <Menu ref="menu" :model="options" :popup="true" />
    </div>
</template>
