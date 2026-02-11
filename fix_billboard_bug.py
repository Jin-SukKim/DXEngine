#!/usr/bin/env python3
"""
Fix billboard batching bug by reordering rendering:
1. Mesh batch first (CS + render + Flush)
2. Billboard batch second (CS + render)
"""

def main():
    filepath = r'DXEngine\ParticleManager.cpp'

    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Find the setup section end
    setup_end = None
    for i, line in enumerate(lines):
        if 'ModelManager::Get().BindBuffersForRender();' in line:
            setup_end = i
            break

    # Find billboard CS block start
    billboard_cs_start = None
    for i in range(setup_end, len(lines)):
        if '// Dispatch billboard batch compute shader' in line:
            billboard_cs_start = i
            break

    # Find where billboard CS block ends (where mesh CS starts)
    mesh_cs_start = None
    for i in range(billboard_cs_start + 1, len(lines)):
        if '// Dispatch mesh batch compute shader' in line:
            mesh_cs_start = i
            break

    # Find mesh rendering block
    mesh_render_start = None
    for i in range(mesh_cs_start + 1, len(lines)):
        if '// 1. Mesh RenderModule' in line:
            mesh_render_start = i
            break

    # Find mesh rendering block end (look for the unbind + closing brace)
    mesh_render_end = None
    if mesh_render_start:
        brace_count = 0
        for i in range(mesh_render_start, len(lines)):
            line = lines[i]
            if 'if (!m_meshBatches.empty())' in line:
                brace_count = 1
            elif brace_count > 0:
                brace_count += line.count('{')
                brace_count -= line.count('}')
                if brace_count == 0:
                    mesh_render_end = i
                    break

    # Find where to continue after mesh render
    continue_after = None
    for i in range(mesh_render_end + 1, len(lines)):
        if 'm_memoryPool->UnbindRender();' in line:
            continue_after = i
            break

    print(f"Setup ends at: {setup_end}")
    print(f"Billboard CS: {billboard_cs_start} to {mesh_cs_start - 1}")
    print(f"Mesh CS: {mesh_cs_start} to {mesh_render_start - 1}")
    print(f"Mesh Render: {mesh_render_start} to {mesh_render_end}")
    print(f"Continue after: {continue_after}")

    if not all([setup_end, billboard_cs_start, mesh_cs_start, mesh_render_start, mesh_render_end, continue_after]):
        print("ERROR: Could not find all sections")
        return

    # Extract sections
    prefix = lines[:setup_end + 1]
    prefix.append('\n')

    # Extract billboard CS (lines billboard_cs_start to mesh_cs_start - 1)
    billboard_cs_raw = lines[billboard_cs_start:mesh_cs_start]

    # Remove billboard rendering from within billboard CS
    # Find "// Render billboards immediately" and remove until end of if block
    billboard_cs = []
    skip_mode = False
    skip_depth = 0

    for line in billboard_cs_raw:
        if '// Render billboards immediately' in line or 'GET_SINGLE(RenderBase)->SetLowResRender();' in line:
            skip_mode = True
            skip_depth = 0
            continue

        if skip_mode:
            skip_depth += line.count('{')
            skip_depth -= line.count('}')
            if skip_depth < 0:  # We've closed all braces from the rendering block
                skip_mode = False
                skip_depth = 0
            continue

        billboard_cs.append(line)

    # Extract mesh CS (lines mesh_cs_start to mesh_render_start - 1)
    mesh_cs = lines[mesh_cs_start:mesh_render_start]

    # Extract mesh rendering (mesh_render_start to mesh_render_end)
    mesh_render = lines[mesh_render_start:mesh_render_end + 1]

    # Modify mesh render to add Flush() before the closing brace
    # Find the last line with the closing brace
    for i in range(len(mesh_render) - 1, -1, -1):
        if '}' in mesh_render[i] and 'VSSetShaderResources' not in mesh_render[i]:
            # Insert flush before this closing brace
            mesh_render.insert(i, '\n')
            mesh_render.insert(i + 1, '\t\t// ★ GPU SYNC: Force completion before billboard upload overwrites shared buffers\n')
            mesh_render.insert(i + 2, '\t\tcontext->Flush();\n')
            break

    # Create billboard rendering block
    billboard_render = [
        '\n',
        '\t// Render billboard particles (alpha blending with depth testing)\n',
        '\tGET_SINGLE(RenderBase)->SetLowResRender();\n',
        '\tif (!m_billboardBatches.empty()) {\n',
        '\t\tGET_SINGLE(RenderBase)->SetPipelineState(RenderBase::graphicsCommon.particle.billboardInstancedPSO);\n',
        '\t\tcontext->OMSetBlendState(RenderBase::graphicsCommon.accumulateBS.Get(), RenderBase::graphicsCommon.particle.animPSO.blendFactor, 0xffffffff);\n',
        '\n',
        '\t\t// Bind batch resources for vertex shader\n',
        '\t\tID3D11ShaderResourceView* batchSRVs[] = {\n',
        '\t\t\tm_memoryPool->GetBatchEmitterList().GetSRV(),\n',
        '\t\t\tm_memoryPool->GetBatchDescriptors().GetSRV()\n',
        '\t\t};\n',
        '\t\tcontext->VSSetShaderResources(24, 2, batchSRVs);\n',
        '\n',
        '\t\tfor (size_t batchIdx = 0; batchIdx < m_billboardBatches.size(); batchIdx++) {\n',
        '\t\t\tconst auto& batch = m_billboardBatches[batchIdx];\n',
        '\t\t\tconst auto& desc = billboardBatchDescriptors[batchIdx];\n',
        '\n',
        '\t\t\t// Bind material based on materialKey\n',
        '\t\t\tif (batch.materialKey < 0) {\n',
        '\t\t\t\t// No material - use default circle rendering\n',
        '\t\t\t\tm_memoryPool->BindDefaultParticleMaterial();\n',
        '\t\t\t} else {\n',
        '\t\t\t\t// Has material - bind normally\n',
        '\t\t\t\tMaterialSystem::Get().BindMaterial(batch.materialKey);\n',
        '\t\t\t}\n',
        '\n',
        '\t\t\t// Bind batch info to CB5 (replaces BindEmitterID)\n',
        '\t\t\tm_memoryPool->BindBatchInfo(desc.emitterCount, desc.emitterListOffset);\n',
        '\n',
        '\t\t\t// Single draw call for entire batch\n',
        '\t\t\tID3D11Buffer* batchArgs = m_memoryPool->GetBatchBillboardArgs().GetBuffer();\n',
        '\t\t\tcontext->DrawIndexedInstancedIndirect(batchArgs, batchIdx * 20);\n',
        '\t\t}\n',
        '\n',
        '\t\t// Unbind batch SRVs\n',
        '\t\tID3D11ShaderResourceView* nullBillboardSRVs[2] = { nullptr };\n',
        '\t\tcontext->VSSetShaderResources(24, 2, nullBillboardSRVs);\n',
        '\t}\n',
    ]

    # Suffix is everything after mesh_render_end
    suffix = lines[continue_after:]

    # Reconstruct file:
    # prefix + mesh section + billboard section + billboard render + suffix
    new_content = []
    new_content.extend(prefix)
    new_content.append('\t// ========== MESH BATCH FIRST (Fills depth buffer) ==========\n')
    new_content.append('\n')
    new_content.extend(mesh_cs)
    new_content.append('\n')
    new_content.append('\t// Render mesh particles (depth buffer 채우기) - BATCHED\n')
    new_content.extend(mesh_render)
    new_content.append('\n')
    new_content.append('\t// ========== BILLBOARD BATCH SECOND (Alpha blending with depth testing) ==========\n')
    new_content.append('\n')
    new_content.extend(billboard_cs)
    new_content.extend(billboard_render)
    new_content.append('\n')
    new_content.append('\n')
    new_content.extend(suffix)

    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        f.writelines(new_content)

    print("\nSuccessfully fixed billboard batching bug!")
    print("Changes made:")
    print("  1. Moved mesh batch to execute FIRST")
    print("  2. Added context->Flush() after mesh rendering")
    print("  3. Moved billboard batch to execute SECOND")
    print("  4. Separated billboard rendering from CS block")

if __name__ == '__main__':
    main()
