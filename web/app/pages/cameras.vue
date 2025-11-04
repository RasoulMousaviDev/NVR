<script setup>
definePageMeta({ path: '/' })

const { data, pending: fetching, refresh, clear } = useLazyFetch('/api/cameras')
</script>

<template>
    <div class="bg-white rounded p-6">
        <DataTable :value="data?.items">
            <template #header>
                <CameraHeader :fetching @refresh="refresh" />
            </template>
            <template #empty>
                <p class="text-center text-sm opacity-60">
                    {{ fetching ? $t('loading') : $t('not-found') }}
                </p>
            </template>
            <Column field="model" :header="$t('model')"></Column>
            <Column field="ip" :header="$t('ip')"></Column>
            <Column field="serial_number" :header="$t('serial-number')"></Column>
            <Column field="manufacturer" :header="$t('manufacturer')"></Column>
            <Column field="firmware_version" :header="$t('firmware-version')"></Column>
            <Column :header="$t('recording-status')">
                <template #body="{ data }">
                    <div class="flex items-center w-full">
                        <InputSwitch v-model="data.recording" @change="handleRecord(data.recording)" />
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
                        <Button icon="pi pi-folder" text rounded v-tooltip.top="$t('archive')" />
                        <Button icon="pi pi-play-circle" text rounded v-tooltip.top="$t('live-stream')" />
                    </div>
                </template>
            </Column>
        </DataTable>
    </div>
</template>
