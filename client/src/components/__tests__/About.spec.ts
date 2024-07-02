import { describe, it, expect } from 'vitest'

import { mount } from '@vue/test-utils'
import About from '../../views/AboutView.vue'

describe('About', () => {
  it('renders the given title', () => {
    const wrapper = mount(About, { props: { msg: 'Ingredients' } })
    expect(wrapper.text()).toContain('Ingredients')
  })
})
